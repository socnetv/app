// SPDX-License-Identifier: GPL-3.0-or-later
// SocNetV - Social Network Visualizer
//
// Clustering kernel (schema v6) for socnetv-cli.
// Covers clustering coefficients + triad census + clique census.

#include "kernel_clustering_v6.h"

#include "graph.h"
#include "graphvertex.h"
#include "tools/cli/cli_common.h"
#include "tools/headless_graph_loader.h"

#include <QElapsedTimer>
#include <QFileInfo>
#include <QJsonArray>
#include <QMap>
#include <QSet>
#include <QTextStream>

namespace cli
{

    // ------------------------------
    // Per-node builder
    // ------------------------------

    static QJsonArray buildPerNodeArrayV6(Graph &g)
    {
        QJsonArray arr;

        const QList<int> verts = g.verticesList(); // deterministic order
        for (int v : verts)
        {
            GraphVertex *gv = g.vertexPtr(v);
            if (!gv)
                continue;

            QJsonObject o;
            o["id"] = v;
            o["label"] = gv->label();
            o["CLC"] = d2s(gv->CLC());
            o["hasCLC"] = gv->hasCLC();

            arr.append(o);
        }

        return arr;
    }

    // ------------------------------
    // Metrics builder
    // ------------------------------

    static QJsonObject buildMetricsV6(Graph &g, const qreal averageCLC)
    {
        QJsonObject metrics;

        int nodesWithCLC = 0;
        const QList<int> verts = g.verticesList();
        for (int v : verts)
        {
            GraphVertex *gv = g.vertexPtr(v);
            if (gv && gv->hasCLC())
                ++nodesWithCLC;
        }

        metrics["averageCLC"] = d2s(averageCLC);
        metrics["nodesWithCLC"] = nodesWithCLC;

        return metrics;
    }

    // ------------------------------
    // Triad census builder
    // ------------------------------

    static QJsonObject buildTriadCensusV6(Graph &g)
    {
        // IMPORTANT:
        // graph_triad_census.cpp documents the canonical storage order:
        // 0:003 1:012 2:102 3:021D 4:021U 5:021C 6:111D 7:111U
        // 8:030T 9:030C 10:201 11:120D 12:120U 13:120C 14:210 15:300

        const QList<int> &f = g.graphTriadTypeFreqs();

        QJsonObject classes;
        classes["003"] = (f.size() > 0) ? f[0] : 0;
        classes["012"] = (f.size() > 1) ? f[1] : 0;
        classes["102"] = (f.size() > 2) ? f[2] : 0;
        classes["021D"] = (f.size() > 3) ? f[3] : 0;
        classes["021U"] = (f.size() > 4) ? f[4] : 0;
        classes["021C"] = (f.size() > 5) ? f[5] : 0;
        classes["111D"] = (f.size() > 6) ? f[6] : 0;
        classes["111U"] = (f.size() > 7) ? f[7] : 0;
        classes["030T"] = (f.size() > 8) ? f[8] : 0;
        classes["030C"] = (f.size() > 9) ? f[9] : 0;
        classes["201"] = (f.size() > 10) ? f[10] : 0;
        classes["120D"] = (f.size() > 11) ? f[11] : 0;
        classes["120U"] = (f.size() > 12) ? f[12] : 0;
        classes["120C"] = (f.size() > 13) ? f[13] : 0;
        classes["210"] = (f.size() > 14) ? f[14] : 0;
        classes["300"] = (f.size() > 15) ? f[15] : 0;

        int totalTriads = 0;
        for (int x : f)
            totalTriads += x;

        QJsonObject root;
        root["classes"] = classes;
        root["total_triads"] = totalTriads;

        return root;
    }

    // ------------------------------
    // Clique census builder
    // ------------------------------

    static QJsonObject buildCliquesV6(Graph &g)
    {
        QJsonObject root;
        QJsonObject bySize;

        int totalCliques = 0;
        int maxCliqueSize = 0;

        // Maximal cliques are stored by size in m_cliques and exposed via
        // graphCliquesOfSize(size). We build a stable size->count map.
        const int maxPossibleSize = g.vertices();

        for (int s = 1; s <= maxPossibleSize; ++s)
        {
            const int count = g.graphCliquesOfSize(s);
            if (count <= 0)
                continue;

            bySize[QString::number(s)] = count;
            totalCliques += count;
            maxCliqueSize = s;
        }

        root["by_size"] = bySize;
        root["max_clique_size"] = maxCliqueSize;
        root["total_cliques"] = totalCliques;

        return root;
    }

    // ------------------------------
    // JSON builder
    // ------------------------------

    static QJsonObject buildGoldenJsonV6(
        const QString &inputPath,
        int fileFormat,
        const HeadlessLoadResult &load,
        Graph &g,
        const CliConfig &cfg,
        const qreal averageCLC)
    {
        QJsonObject root;
        root["schema_version"] = 6;
        root["kernel"] = "clustering";

        QJsonObject dataset;
        dataset["path"] = inputPath;
        dataset["name"] = QFileInfo(inputPath).fileName();
        dataset["filetype"] = fileFormat;
        root["dataset"] = dataset;

        QJsonObject run;
        run["considerWeights"] = cfg.considerWeights;
        run["inverseWeights"] = cfg.inverseWeights;
        run["dropIsolates"] = cfg.dropIsolates;
        root["run"] = run;

        const int ties_graph = load.tiesGraph; // canonical
        const int links_sna = g.isDirected() ? ties_graph : (2 * ties_graph);

        QJsonObject counts;
        counts["nodes"] = load.totalNodes;
        counts["links_sna"] = links_sna;
        counts["ties_graph"] = ties_graph;
        root["counts"] = counts;

        QJsonObject graph;
        graph["directed"] = g.isDirected();
        graph["weighted"] = g.isWeighted();
        root["graph"] = graph;

        root["metrics"] = buildMetricsV6(g, averageCLC);
        root["per_node"] = buildPerNodeArrayV6(g);
        root["triad_census"] = buildTriadCensusV6(g);
        root["cliques"] = buildCliquesV6(g);

        QJsonObject loadReport;
        loadReport["ok"] = load.ok;
        loadReport["fileType_signal"] = load.fileType;
        loadReport["load_ms"] = static_cast<qint64>(load.elapsedTime);
        loadReport["load_msg"] = load.message;
        loadReport["net_name"] = load.netName;
        root["load_report"] = loadReport;

        return root;
    }

    // ------------------------------
    // Per-node comparator
    // ------------------------------

    static bool cmpPerNodeArrayV6(const QJsonArray &eArr, const QJsonArray &aArr, QTextStream &err)
    {
        if (eArr.size() != aArr.size())
        {
            err << "MISMATCH per_node.size expected=" << eArr.size()
                << " got=" << aArr.size() << "\n";
            return false;
        }

        bool ok = true;

        auto cmpNodeFieldStrictStr = [&](const QJsonObject &e, const QJsonObject &a, const QString &k, int id)
        {
            const QString ev = e.value(k).toString();
            const QString av = a.value(k).toString();
            if (ev != av)
            {
                err << "MISMATCH per_node id=" << id << " field=" << k
                    << " expected=" << ev << " got=" << av << "\n";
                ok = false;
            }
        };

        auto cmpNodeFieldNumStrTol = [&](const QJsonObject &e, const QJsonObject &a, const QString &k, int id, double tol)
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

            if (!almostEqual(ev, av, tol))
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
                err << "MISMATCH per_node id=" << id << " field=" << k
                    << " expected=" << ev << " got=" << av << "\n";
                ok = false;
            }
        };

        auto cmpNodeFieldBool = [&](const QJsonObject &e, const QJsonObject &a, const QString &k, int id)
        {
            const bool ev = e.value(k).toBool();
            const bool av = a.value(k).toBool();
            if (ev != av)
            {
                err << "MISMATCH per_node id=" << id << " field=" << k
                    << " expected=" << (ev ? 1 : 0) << " got=" << (av ? 1 : 0) << "\n";
                ok = false;
            }
        };

        constexpr double TOL = 1e-15;

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
            cmpNodeFieldNumStrTol(e, a, "CLC", eid, TOL);
            cmpNodeFieldBool(e, a, "hasCLC", eid);
        }

        return ok;
    }

    // ------------------------------
    // Triad comparator
    // ------------------------------

    static bool cmpTriadClassesV6(const QJsonObject &eObj, const QJsonObject &aObj, QTextStream &err)
    {
        bool ok = true;

        const QStringList fields = {
            "003", "012", "102",
            "021D", "021U", "021C",
            "111D", "111U",
            "030T", "030C",
            "201",
            "120D", "120U", "120C",
            "210", "300"};

        for (const QString &f : fields)
            ok &= cmpInt(eObj, aObj, f, err);

        return ok;
    }

    static bool cmpCliquesBySizeV6(const QJsonObject &eObj, const QJsonObject &aObj, QTextStream &err)
    {
        bool ok = true;

        const QStringList eKeys = eObj.keys();
        const QStringList aKeys = aObj.keys();

        if (eKeys.size() != aKeys.size())
        {
            err << "MISMATCH cliques.by_size.size expected=" << eKeys.size()
                << " got=" << aKeys.size() << "\n";
            ok = false;
        }

        for (const QString &k : eKeys)
        {
            if (!aObj.contains(k))
            {
                err << "MISMATCH cliques.by_size missing key=" << k << "\n";
                ok = false;
                continue;
            }
            ok &= cmpInt(eObj, aObj, k, err);
        }

        return ok;
    }

    // ------------------------------
    // schema v6 compare
    // ------------------------------

    static int compareGoldenV6(const QJsonObject &expected, const QJsonObject &actual)
    {
        QTextStream err(stderr);

        if (expected.value("schema_version").toInt() != 6 ||
            actual.value("schema_version").toInt() != 6)
        {
            err << "ERROR: schema_version mismatch or unsupported\n";
            return 2;
        }

        bool ok = true;

        ok &= cmpStr(expected, actual, "kernel", err); // should be "clustering"

        const QJsonObject eDataset = expected.value("dataset").toObject();
        const QJsonObject aDataset = actual.value("dataset").toObject();
        ok &= cmpInt(eDataset, aDataset, "filetype", err);
        ok &= cmpStr(eDataset, aDataset, "name", err);

        const QJsonObject eRun = expected.value("run").toObject();
        const QJsonObject aRun = actual.value("run").toObject();
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
        ok &= cmpNumStrTol(eMetrics, aMetrics, "averageCLC", err);
        ok &= cmpInt(eMetrics, aMetrics, "nodesWithCLC", err);

        const QJsonArray ePN = expected.value("per_node").toArray();
        const QJsonArray aPN = actual.value("per_node").toArray();
        ok &= cmpPerNodeArrayV6(ePN, aPN, err);

        const QJsonObject eTriads = expected.value("triad_census").toObject();
        const QJsonObject aTriads = actual.value("triad_census").toObject();
        ok &= cmpTriadClassesV6(eTriads.value("classes").toObject(),
                                aTriads.value("classes").toObject(),
                                err);
        ok &= cmpInt(eTriads, aTriads, "total_triads", err);

        const QJsonObject eCliques = expected.value("cliques").toObject();
        const QJsonObject aCliques = actual.value("cliques").toObject();
        ok &= cmpCliquesBySizeV6(eCliques.value("by_size").toObject(),
                                 aCliques.value("by_size").toObject(),
                                 err);
        ok &= cmpInt(eCliques, aCliques, "max_clique_size", err);
        ok &= cmpInt(eCliques, aCliques, "total_cliques", err);

        if (!ok)
            return 1;

        err << "OK: baseline match\n";
        return 0;
    }

    // ------------------------------
    // Runner
    // ------------------------------

    int runKernelClusteringV6(const CliConfig &cfg,
                              const HeadlessLoadResult &load,
                              Graph &g)
    {
        QElapsedTimer t;
        t.start();

        const qreal averageCLC = g.clusteringCoefficient(false);

        if (!g.graphTriadCensus())
        {
            QTextStream(stderr) << "ERROR: graphTriadCensus failed\n";
            return 2;
        }

        g.graphCliques(QSet<int>(), QSet<int>(), QSet<int>());

        const qint64 computeMs = t.elapsed();
        printKV("COMPUTE_MS", computeMs);

        const QJsonObject actual =
            buildGoldenJsonV6(cfg.inputPath,
                              cfg.fileFormat,
                              load,
                              g,
                              cfg,
                              averageCLC);

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
            return compareGoldenV6(expected, actual);
        }

        return 0;
    }

} // namespace cli