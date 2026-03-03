// SPDX-License-Identifier: GPL-3.0-or-later
//
// IO Roundtrip kernel (schema v5) for socnetv-cli.
// Multirelational support:
//   - Build canonical signature PER RELATION
//   - Roundtrip equivalence requires identical relations bundle
//   - Reports relation count in JSON + harness prints RELATIONS in facade
//
// NOTE: For Pajek multirel matrices, roundtrip currently fails because exporter
// likely writes only the current relation. This is a real IO regression signal.

#include "kernel_io_roundtrip_v5.h"

#include "graph.h"
#include "graphvertex.h"
#include "tools/headless_graph_loader.h"

#include <QCryptographicHash>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QSet>
#include <QTemporaryDir>
#include <QTextStream>

#include <algorithm>

namespace cli
{

    namespace
    {

        // -------------------------------
        // Helpers
        // -------------------------------

        static QByteArray sha256(const QByteArray &bytes)
        {
            return QCryptographicHash::hash(bytes, QCryptographicHash::Sha256);
        }

        static QJsonObject hashToJsonObjectSorted(const QHash<QString, QString> &h)
        {
            QMap<QString, QString> sorted;
            for (auto it = h.begin(); it != h.end(); ++it)
                sorted.insert(it.key(), it.value());

            QJsonObject o;
            for (auto it = sorted.begin(); it != sorted.end(); ++it)
                o.insert(it.key(), it.value());
            return o;
        }

        static QString suffixForFileType(int ft)
        {
            // Matches global.h enum
            switch (ft)
            {
            case FileType::GRAPHML:
                return ".graphml";
            case FileType::PAJEK:
                return ".paj";
            case FileType::ADJACENCY:
                return ".adj";
            case FileType::GRAPHVIZ:
                return ".dot";
            case FileType::UCINET:
                return ".dl";
            case FileType::GML:
                return ".gml";
            default:
                return ".tmp";
            }
        }

        struct EdgeRow
        {
            int u = 0;
            int v = 0;
            QString w;     // deterministic string
            QString label; // may be empty
        };

        static bool edgeLess(const EdgeRow &a, const EdgeRow &b)
        {
            if (a.u != b.u)
                return a.u < b.u;
            if (a.v != b.v)
                return a.v < b.v;
            if (a.w != b.w)
                return a.w < b.w;
            return a.label < b.label;
        }

        static void printSigMismatchHint(const QJsonObject &sig1, const QJsonObject &sig2)
        {
            QTextStream err(stderr);
            err << "SIG1_SHA256=" << sig1.value("hash_sha256").toString() << "\n";
            err << "SIG2_SHA256=" << sig2.value("hash_sha256").toString() << "\n";

            const QJsonArray e1 = sig1.value("edge_list").toArray();
            const QJsonArray e2 = sig2.value("edge_list").toArray();
            if (e1 != e2)
            {
                err << "HINT=edge_list differ\n";
                const int n = qMin(e1.size(), e2.size());
                for (int i = 0; i < n; ++i)
                {
                    const QJsonObject o1 = e1.at(i).toObject();
                    const QJsonObject o2 = e2.at(i).toObject();
                    if (o1 != o2)
                    {
                        err << "FIRST_DIFF edge_list[" << i << "]\n";
                        err << "EXPECTED=" << QJsonDocument(o1).toJson(QJsonDocument::Compact) << "\n";
                        err << "GOT=" << QJsonDocument(o2).toJson(QJsonDocument::Compact) << "\n";
                        return;
                    }
                }
                err << "FIRST_DIFF edge_list size expected=" << e1.size()
                    << " got=" << e2.size() << "\n";
                return;
            }

            const QJsonArray l1 = sig1.value("node_labels").toArray();
            const QJsonArray l2 = sig2.value("node_labels").toArray();
            if (l1 != l2)
            {
                err << "HINT=node_labels differ\n";
                const int n = qMin(l1.size(), l2.size());
                for (int i = 0; i < n; ++i)
                {
                    const QString a = l1.at(i).toString();
                    const QString b = l2.at(i).toString();
                    if (a != b)
                    {
                        err << "FIRST_DIFF node_labels[" << i << "] expected=" << a
                            << " got=" << b << "\n";
                        return;
                    }
                }
                err << "FIRST_DIFF node_labels size expected=" << l1.size()
                    << " got=" << l2.size() << "\n";
                return;
            }

            const QJsonArray a1 = sig1.value("node_custom_attributes").toArray();
            const QJsonArray a2 = sig2.value("node_custom_attributes").toArray();
            if (a1 != a2)
            {
                err << "HINT=node_custom_attributes differ\n";
                err << "FIRST_DIFF node_custom_attributes differs (array mismatch)\n";
                return;
            }

            err << "HINT=signature differs but no localized diff found (unexpected)\n";
        }

        // -------------------------------
        // Signature for CURRENT relation
        // -------------------------------

        static QJsonObject buildSignatureForCurrentRelation(Graph &g)
        {
            QList<int> order = g.verticesList();
            std::sort(order.begin(), order.end());

            QJsonArray nodeLabels;
            for (int id : order)
                nodeLabels.append(g.vertexLabel(id));

            QJsonArray nodeAttrs;
            for (int id : order)
                nodeAttrs.append(hashToJsonObjectSorted(g.vertexCustomAttributes(id)));

            QList<EdgeRow> edges;
            edges.reserve(g.edgesEnabled());

            QSet<qulonglong> seenUndir;

            for (int u : order)
            {
                GraphVertex *gv = g.vertexPtr(u);
                if (!gv)
                    continue;

                // current relation only (as per GraphVertex state after relationSet)
                const QHash<int, qreal> out = gv->outEdgesEnabledHash(false);
                for (auto it = out.begin(); it != out.end(); ++it)
                {
                    const int v = it.key();
                    const qreal w = it.value();

                    int uu = u, vv = v;
                    if (!g.isDirected())
                    {
                        uu = qMin(u, v);
                        vv = qMax(u, v);

                        const qulonglong key =
                            (static_cast<qulonglong>(static_cast<quint32>(uu)) << 32) |
                            static_cast<qulonglong>(static_cast<quint32>(vv));

                        if (seenUndir.contains(key))
                            continue;
                        seenUndir.insert(key);
                    }

                    EdgeRow row;
                    row.u = uu;
                    row.v = vv;
                    row.w = d2s(static_cast<double>(w));
                    row.label = g.edgeLabel(uu, vv);

                    edges.push_back(row);
                }
            }

            std::sort(edges.begin(), edges.end(), edgeLess);

            QJsonArray edgeList;
            for (const auto &e : edges)
            {
                QJsonObject o;
                o["u"] = e.u;
                o["v"] = e.v;
                o["w"] = e.w;
                if (!e.label.isEmpty())
                    o["label"] = e.label;
                edgeList.append(o);
            }

            QJsonObject sig;
            sig["node_labels"] = nodeLabels;
            sig["node_custom_attributes"] = nodeAttrs;
            sig["edge_list"] = edgeList;

            const QByteArray canon = QJsonDocument(sig).toJson(QJsonDocument::Compact);
            sig["hash_sha256"] = QString::fromLatin1(sha256(canon).toHex());

            return sig;
        }

        // -------------------------------
        // Per-relation bundle
        // -------------------------------

        static QJsonArray buildRelationsBundle(Graph &g)
        {
            QJsonArray arr;

            const int relCount = g.relations();
            const int originalRel = g.relationCurrent();

            for (int r = 0; r < relCount; ++r)
            {
                g.relationSet(r, false /* updateUI */);

                QJsonObject relObj;
                relObj["index"] = r;
                relObj["name"] = g.relationCurrentName();
                relObj["ties_graph"] = g.edgesEnabled(); // canonical ties for this relation
                relObj["signature"] = buildSignatureForCurrentRelation(g);

                arr.append(relObj);
            }

            // restore
            if (originalRel >= 0 && originalRel < relCount)
                g.relationSet(originalRel, false);

            return arr;
        }

        static bool compareRelationsBundle(const QJsonArray &expected,
                                           const QJsonArray &actual,
                                           QTextStream &err)
        {
            if (expected.size() != actual.size())
            {
                err << "MISMATCH roundtrip.relations.size expected=" << expected.size()
                    << " got=" << actual.size() << "\n";
                return false;
            }

            for (int i = 0; i < expected.size(); ++i)
            {
                const QJsonObject e = expected.at(i).toObject();
                const QJsonObject a = actual.at(i).toObject();

                if (e.value("index").toInt() != a.value("index").toInt())
                {
                    err << "MISMATCH relations[" << i << "].index expected="
                        << e.value("index").toInt() << " got=" << a.value("index").toInt() << "\n";
                    return false;
                }

                if (e.value("name").toString() != a.value("name").toString())
                {
                    err << "MISMATCH relations[" << i << "].name expected="
                        << e.value("name").toString() << " got=" << a.value("name").toString() << "\n";
                    return false;
                }

                if (e.value("ties_graph").toInt() != a.value("ties_graph").toInt())
                {
                    err << "MISMATCH relations[" << i << "].ties_graph expected="
                        << e.value("ties_graph").toInt() << " got=" << a.value("ties_graph").toInt() << "\n";
                    return false;
                }

                const QJsonObject es = e.value("signature").toObject();
                const QJsonObject as = a.value("signature").toObject();
                if (es != as)
                {
                    err << "MISMATCH relations[" << i << "].signature differs\n";
                    printSigMismatchHint(es, as);
                    return false;
                }
            }

            return true;
        }

        // -------------------------------
        // Golden JSON builder + compare (schema v5)
        // -------------------------------

        static QJsonObject buildGoldenJsonV5Io(const CliConfig &cfg,
                                               const HeadlessLoadResult &load,
                                               Graph &g,
                                               const QJsonObject &sig_current_relation,
                                               const QJsonArray &relations_bundle,
                                               bool performed,
                                               bool equivalent,
                                               qint64 saveMs,
                                               qint64 reloadMs,
                                               qint64 totalMs)
        {
            QJsonObject root;
            root["schema_version"] = 5;
            root["kernel"] = "io_roundtrip";

            QJsonObject dataset;
            dataset["path"] = cfg.inputPath;
            dataset["name"] = QFileInfo(cfg.inputPath).fileName();
            dataset["filetype"] = cfg.fileFormat;
            root["dataset"] = dataset;

            QJsonObject run;
            run["considerWeights"] = cfg.considerWeights;
            run["inverseWeights"] = cfg.inverseWeights;
            run["dropIsolates"] = cfg.dropIsolates;
            run["operation"] = "io_roundtrip_same_format";
            root["run"] = run;

            QJsonObject graph;
            graph["directed"] = g.isDirected();
            graph["weighted"] = g.isWeighted();
            graph["relations"] = g.relations();
            root["graph"] = graph;

            // For continuity with existing CLI semantics:
            // counts are derived from the currently selected relation (usually relation 0 after load).
            const int ties_graph = load.tiesGraph;
            const int links_sna = g.isDirected() ? ties_graph : (2 * ties_graph);

            QJsonObject counts;
            counts["nodes"] = load.totalNodes;
            counts["links_sna"] = links_sna;
            counts["ties_graph"] = ties_graph;
            root["counts"] = counts;

            QJsonObject metrics;
            metrics["density"] = d2s(g.graphDensity());
            root["metrics"] = metrics;

            QJsonObject timings;
            timings["load_ms"] = static_cast<qint64>(load.elapsedTime);
            timings["save_ms"] = static_cast<qint64>(saveMs);
            timings["reload_ms"] = static_cast<qint64>(reloadMs);
            timings["total_ms"] = static_cast<qint64>(totalMs);
            root["timings"] = timings;

            // v5 existing key (current relation signature)
            root["signature"] = sig_current_relation;

            // NEW in v5 (additive, no schema bump): multirel bundle
            root["relations_bundle"] = relations_bundle;

            QJsonObject rt;
            rt["performed"] = performed;
            rt["equivalent"] = equivalent;
            root["roundtrip_result"] = rt;

            QJsonObject loadReport;
            loadReport["ok"] = load.ok;
            loadReport["fileType_signal"] = load.fileType;
            loadReport["load_ms"] = static_cast<qint64>(load.elapsedTime);
            loadReport["load_msg"] = load.message;
            loadReport["net_name"] = load.netName;
            root["load_report"] = loadReport;

            return root;
        }

        static int checkIoTimings(const QJsonObject &expected,
                                  const QJsonObject &actual,
                                  bool strict,
                                  QTextStream &err)
        {
            // Timing check policy:
            // - N < 50: skip entirely (measurement noise dominates)
            // - baseline load_ms == 0: skip (can't compute threshold)
            // - N >= 50: warn if actual > baseline * 110%; fail only if --strict

            const int n = expected.value("counts").toObject().value("nodes").toInt(0);
            if (n < 50)
            {
                err << "TIMING_SKIP: N=" << n << " < 50, timing not enforced\n";
                return 0;
            }

            const qint64 baselineMs = static_cast<qint64>(
                expected.value("timings").toObject().value("load_ms").toInteger(0));
            if (baselineMs == 0)
            {
                err << "TIMING_SKIP: baseline load_ms=0, timing not enforced\n";
                return 0;
            }

            const qint64 actualMs = static_cast<qint64>(
                actual.value("timings").toObject().value("load_ms").toInteger(0));

            const qint64 allowed = baselineMs + (baselineMs / 10); // +10%

            if (actualMs <= allowed)
            {
                err << "TIMING_OK: load_ms=" << actualMs
                    << " <= " << allowed << " (baseline=" << baselineMs << " +10%)\n";
                return 0;
            }

            err << "TIMING_WARN: load_ms=" << actualMs
                << " > " << allowed << " (baseline=" << baselineMs << " +10%)\n";

            if (strict)
            {
                err << "TIMING_FAIL: --strict mode, timing regression is a hard failure\n";
                return 1;
            }
            return 0;
        }


        static int compareGoldenV5Io(const QJsonObject &expected, const QJsonObject &actual, const CliConfig &cfg)
        {
            QTextStream err(stderr);

            if (expected.value("schema_version").toInt() != 5 || actual.value("schema_version").toInt() != 5)
            {
                err << "ERROR: schema_version mismatch or unsupported\n";
                return 2;
            }
            if (expected.value("kernel").toString() != "io_roundtrip" ||
                actual.value("kernel").toString() != "io_roundtrip")
            {
                err << "ERROR: kernel mismatch or unsupported\n";
                return 2;
            }

            bool ok = true;

            const QJsonObject eDs = expected.value("dataset").toObject();
            const QJsonObject aDs = actual.value("dataset").toObject();
            ok &= cmpInt(eDs, aDs, "filetype", err);
            ok &= cmpStr(eDs, aDs, "name", err);

            const QJsonObject eRun = expected.value("run").toObject();
            const QJsonObject aRun = actual.value("run").toObject();
            ok &= cmpBool(eRun, aRun, "considerWeights", err);
            ok &= cmpBool(eRun, aRun, "inverseWeights", err);
            ok &= cmpBool(eRun, aRun, "dropIsolates", err);
            ok &= cmpStr(eRun, aRun, "operation", err);

            const QJsonObject eG = expected.value("graph").toObject();
            const QJsonObject aG = actual.value("graph").toObject();
            ok &= cmpBool(eG, aG, "directed", err);
            ok &= cmpBool(eG, aG, "weighted", err);
            ok &= cmpInt(eG, aG, "relations", err);

            const QJsonObject eC = expected.value("counts").toObject();
            const QJsonObject aC = actual.value("counts").toObject();
            ok &= cmpInt(eC, aC, "nodes", err);
            ok &= cmpInt(eC, aC, "links_sna", err);
            ok &= cmpInt(eC, aC, "ties_graph", err);

            const QJsonObject eM = expected.value("metrics").toObject();
            const QJsonObject aM = actual.value("metrics").toObject();
            ok &= cmpNumStrTol(eM, aM, "density", err, 0.0, 0.0);

            const QJsonObject eRt = expected.value("roundtrip_result").toObject();
            const QJsonObject aRt = actual.value("roundtrip_result").toObject();
            ok &= cmpBool(eRt, aRt, "performed", err);
            ok &= cmpBool(eRt, aRt, "equivalent", err);

            if (!ok)
                return 1;

            // Compare multirel bundle strictly (always present in v5 output now).
            const QJsonArray eRel = expected.value("relations_bundle").toArray();
            const QJsonArray aRel = actual.value("relations_bundle").toArray();
            if (!compareRelationsBundle(eRel, aRel, err))
                return 1;

            // Timing check (advisory unless --strict)
            const int timingRc = checkIoTimings(expected, actual, cfg.strict, err);
            if (timingRc != 0)
                return timingRc;

            err << "OK: baseline match\n";
            return 0;
        }

    } // namespace

    // -------------------------------
    // exported runner
    // -------------------------------

    int runKernelIoRoundtripV5(const CliConfig &cfg,
                               const HeadlessLoadResult &load,
                               Graph &g)
    {
        // IO kernel must not mutate graph (drop isolates is a mutation).
        if (cfg.dropIsolates)
        {
            QTextStream(stderr) << "ERROR: --drop-isolates is not allowed for --kernel io_roundtrip\n";
            return 2;
        }
        if (cfg.benchRuns > 0)
        {
            QTextStream(stderr) << "ERROR: --bench is only supported with --kernel distance\n";
            return 2;
        }

        const bool exportSupported = g.isFileFormatExportSupported(cfg.fileFormat);
        if (!exportSupported)
        {
            QTextStream(stderr) << "ROUNDTRIP_SKIPPED: export not supported for file type "
                                << cfg.fileFormat << "\n";
        }

        QElapsedTimer total;
        total.start();

        // Build multirel bundle from loaded model
        const QJsonArray rel1 = buildRelationsBundle(g);

        // Keep legacy single signature: current relation snapshot
        const QJsonObject sig1_current = buildSignatureForCurrentRelation(g);

        bool performed = false;
        bool equivalent = true;
        qint64 saveMs = 0;
        qint64 reloadMs = 0;

        if (exportSupported)
        {
            performed = true;

            QTemporaryDir tmpDir;
            if (!tmpDir.isValid())
            {
                QTextStream(stderr) << "ERROR: could not create temporary directory for roundtrip\n";
                return 2;
            }

            const QString outPath =
                tmpDir.path() + "/socnetv_cli_roundtrip" + suffixForFileType(cfg.fileFormat);

            // Save (timed)
            QElapsedTimer tSave;
            tSave.start();
            g.saveToFile(outPath, cfg.fileFormat, true /*saveEdgeWeights*/, false /*saveZeroWeightEdges*/);
            saveMs = tSave.elapsed();

            // Reload into fresh graph (timed)
            Graph g2;

            QElapsedTimer tReload;
            tReload.start();
            const HeadlessLoadResult load2 = loadGraphHeadless(
                g2,
                outPath,
                "UTF-8",
                cfg.fileFormat,
                cfg.delimiter,
                cfg.twoMode,
                cfg.hasLabels);
            reloadMs = tReload.elapsed();

            if (!load2.ok)
            {
                QTextStream(stderr) << "ERROR: roundtrip reload failed: " << load2.message << "\n";
                return 1;
            }

            const QJsonArray rel2 = buildRelationsBundle(g2);

            equivalent = (rel1 == rel2);
            if (!equivalent)
            {
                QTextStream(stdout) << "ROUNDTRIP_EQUIV=0\n";
                QTextStream(stderr) << "MISMATCH_DETAIL: relations bundle differs\n";
                QTextStream err(stderr);
                (void)compareRelationsBundle(rel1, rel2, err);
            }
            else {
                QTextStream(stdout) << "ROUNDTRIP_EQUIV=1\n";
            }
        }

        const QJsonObject root = buildGoldenJsonV5Io(cfg, load, g,
                                                     sig1_current,
                                                     rel1,
                                                     performed, equivalent,
                                                     saveMs, reloadMs,
                                                     total.elapsed());

        if (!cfg.dumpJsonPath.isEmpty())
        {
            QString err;
            if (!writeJsonFile(cfg.dumpJsonPath, root, &err))
            {
                QTextStream(stderr) << "ERROR: JSON dump failed: " << err << "\n";
                return 2;
            }
        }

        if (!cfg.compareJsonPath.isEmpty())
        {
            QJsonObject expected;
            QString err;
            if (!readJsonFile(cfg.compareJsonPath, &expected, &err))
            {
                QTextStream(stderr) << "ERROR: JSON compare read failed: " << err << "\n";
                return 2;
            }
            const int rc = compareGoldenV5Io(expected, root, cfg);
            if (rc != 0)
                return rc;
        }

        if (performed && !equivalent)
        {
            QTextStream(stderr) << "MISMATCH: roundtrip differs (schema v5, multirel)\n";
            return 1;
        }

        return 0;
    }

} // namespace cli