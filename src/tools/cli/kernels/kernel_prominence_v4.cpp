// SPDX-License-Identifier: GPL-3.0-or-later
// SocNetV - Social Network Visualizer
//
// Prominence kernel (schema v4) for socnetv-cli.
// Covers ALL centrality + prestige indices stored on GraphVertex.

#include "kernel_prominence_v4.h"

#include "graph.h"
#include "graphvertex.h"
#include "tools/cli/cli_common.h"
#include "tools/headless_graph_loader.h"

#include <QElapsedTimer>
#include <QFileInfo>
#include <QJsonArray>
#include <QTextStream>

namespace cli
{

    // ------------------------------
    // Per-node builder
    // ------------------------------

    static QJsonArray buildPerNodeArrayV4(Graph &g)
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

            // ---- Centrality ----
            o["DC"] = d2s(gv->DC());
            o["SDC"] = d2s(gv->SDC());

            o["CC"] = d2s(gv->CC());
            o["SCC"] = d2s(gv->SCC());

            o["BC"] = d2s(gv->BC());
            o["SBC"] = d2s(gv->SBC());

            o["SC"] = d2s(gv->SC());
            o["SSC"] = d2s(gv->SSC());

            o["PC"] = d2s(gv->PC());
            o["SPC"] = d2s(gv->SPC());

            o["IC"] = d2s(gv->IC());
            o["SIC"] = d2s(gv->SIC());

            o["EVC"] = d2s(gv->EVC());
            o["SEVC"] = d2s(gv->SEVC());

            o["IRCC"] = d2s(gv->IRCC());
            o["SIRCC"] = d2s(gv->SIRCC());

            const qreal ecc = gv->eccentricity();
            o["eccentricity"] = d2s(ecc);
            o["eccentricity_inf"] = (ecc >= 2147483647.0);

            // ---- Prestige ----
            o["DP"] = d2s(gv->DP());
            o["SDP"] = d2s(gv->SDP());

            o["PP"] = d2s(gv->PP());
            o["SPP"] = d2s(gv->SPP());

            o["PRP"] = d2s(gv->PRP());
            o["SPRP"] = d2s(gv->SPRP());

            arr.append(o);
        }

        return arr;
    }

    // ------------------------------
    // JSON builder
    // ------------------------------

    static QJsonObject buildGoldenJsonV4(
        const QString &inputPath,
        int fileFormat,
        const HeadlessLoadResult &load,
        Graph &g,
        const CliConfig &cfg)
    {
        QJsonObject root;
        root["schema_version"] = 4;
        root["kernel"] = "prominence";

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

        const int ties_graph = load.tiesGraph; // canonical, already correct
        // links_sna is the SNA convention: undirected ties are counted as 2 arcs.
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

        // Metrics (add density so golden JSONs carry it)
        QJsonObject metrics;
        metrics["density"] = d2s(g.graphDensity());
        root["metrics"] = metrics;

        root["per_node"] = buildPerNodeArrayV4(g);

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

    // ---- schema v4 compare ----

    static bool cmpPerNodeArrayV4(const QJsonArray &eArr, const QJsonArray &aArr, QTextStream &err)
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

        // Same tolerance policy as v1 unless you want tighter/looser:
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

            // Prominence v4 fields (centralities + prestige + eigenvector + pagerank + IRCC)
            const QStringList numFields = {
                "DC", "SDC",
                "CC", "SCC",
                "IRCC", "SIRCC",
                "BC", "SBC",
                "SC", "SSC",
                "PC", "SPC",
                "IC", "SIC",
                "EVC", "SEVC",
                "DP", "SDP",
                "PP", "SPP",
                "PRP", "SPRP",
                "eccentricity"};

            for (const QString &f : numFields)
                cmpNodeFieldNumStrTol(e, a, f, eid, TOL);

            // Sentinel flag must be exact
            cmpNodeFieldBool(e, a, "eccentricity_inf", eid);
        }

        return ok;
    }

    static int compareGoldenV4(const QJsonObject &expected, const QJsonObject &actual)
    {
        QTextStream err(stderr);

        if (expected.value("schema_version").toInt() != 4 || actual.value("schema_version").toInt() != 4)
        {
            err << "ERROR: schema_version mismatch or unsupported\n";
            return 2;
        }

        bool ok = true;

        ok &= cmpStr(expected, actual, "kernel", err); // should be "prominence"

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

        // Per-node (always present in v4)
        const QJsonArray ePN = expected.value("per_node").toArray();
        const QJsonArray aPN = actual.value("per_node").toArray();
        ok &= cmpPerNodeArrayV4(ePN, aPN, err);

        if (!ok)
            return 1;

        err << "OK: baseline match\n";
        return 0;
    }

    // ------------------------------
    // Runner
    // ------------------------------

    int runKernelProminenceV4(const CliConfig &cfg,
                              const HeadlessLoadResult &load,
                              Graph &g)
    {
        // Force recomputation
        g.resetDistanceCentralityCacheFlags();

        QElapsedTimer t;
        t.start();

        // 1. Geodesic-based centralities
        g.graphDistancesGeodesic(true,
                                 cfg.considerWeights,
                                 cfg.inverseWeights,
                                 cfg.dropIsolates);

        // 2. Standalone centralities
        g.centralityDegree(cfg.considerWeights, cfg.dropIsolates);
        g.centralityInformation(cfg.considerWeights, cfg.dropIsolates);
        g.centralityEigenvector(cfg.considerWeights, cfg.dropIsolates);
        g.centralityClosenessIR(cfg.considerWeights,
                                cfg.inverseWeights,
                                cfg.dropIsolates);

        // 3. Prestige
        g.prestigeDegree(cfg.considerWeights, cfg.dropIsolates);
        g.prestigeProximity(cfg.considerWeights,
                            cfg.inverseWeights,
                            cfg.dropIsolates);
        g.prestigePageRank(cfg.dropIsolates);

        const qint64 computeMs = t.elapsed();
        printKV("COMPUTE_MS", computeMs);

        const QJsonObject actual =
            buildGoldenJsonV4(cfg.inputPath,
                              cfg.fileFormat,
                              load,
                              g,
                              cfg);

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
            return compareGoldenV4(expected, actual);
        }

        return 0;
    }

} // namespace cli