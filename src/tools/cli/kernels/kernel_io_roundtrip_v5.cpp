// SPDX-License-Identifier: GPL-3.0-or-later
//
// IO Roundtrip kernel (schema v5) for socnetv-cli.
// Protects WS4 IO/Parser refactor by validating that:
//
//   load -> (save same-format) -> reload
//
// preserves graph semantics + metadata exactly.
//
// Canonical counts:
//   ties_graph = HeadlessLoadResult::tiesGraph (from Graph model, not parser counters)
//   links_sna  = directed ? ties_graph : 2*ties_graph

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
            // Stable insertion order for JSON output.
            QMap<QString, QString> sorted;
            for (auto it = h.begin(); it != h.end(); ++it)
                sorted.insert(it.key(), it.value());

            QJsonObject o;
            for (auto it = sorted.begin(); it != sorted.end(); ++it)
                o.insert(it.key(), it.value());
            return o;
        }

        // File suffix to avoid exporter/loader paths that depend on extension.
        static QString suffixForFileType(int ft)
        {
            // IMPORTANT: adjust if your FileType numeric ids differ.
            // Based on your usage: -f 2 for Pajek.
            switch (ft)
            {
            case 1:
                return ".graphml"; // GRAPHML
            case 2:
                return ".paj"; // PAJEK
            case 3:
                return ".adj"; // ADJACENCY (if you use ".txt", change accordingly)
            case 4:
                return ".dot"; // GRAPHVIZ
            case 5:
                return ".dl"; // UCINET
            default:
                return ".tmp";
            }
        }

        static void printSigMismatchHint(const QJsonObject &sig1, const QJsonObject &sig2)
        {
            QTextStream err(stderr);
            err << "SIG1_SHA256=" << sig1.value("hash_sha256").toString() << "\n";
            err << "SIG2_SHA256=" << sig2.value("hash_sha256").toString() << "\n";

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
                const int n = qMin(a1.size(), a2.size());
                for (int i = 0; i < n; ++i)
                {
                    const QJsonObject o1 = a1.at(i).toObject();
                    const QJsonObject o2 = a2.at(i).toObject();
                    if (o1 != o2)
                    {
                        err << "FIRST_DIFF node_custom_attributes[" << i << "]\n";
                        err << "EXPECTED=" << QJsonDocument(o1).toJson(QJsonDocument::Compact) << "\n";
                        err << "GOT=" << QJsonDocument(o2).toJson(QJsonDocument::Compact) << "\n";
                        return;
                    }
                }
                err << "FIRST_DIFF node_custom_attributes size expected=" << a1.size()
                    << " got=" << a2.size() << "\n";
                return;
            }

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

            err << "HINT=signature differs but no localized diff found (unexpected)\n";
        }

        struct EdgeRow
        {
            int u = 0;
            int v = 0;
            QString w;     // deterministic numeric string (d2s)
            QString label; // edge label metadata (may be empty)
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

        // -------------------------------
        // Canonical signature (Graph model)
        // -------------------------------

        static QJsonObject buildSignature(Graph &g)
        {
            // Deterministic vertex order.
            QList<int> order = g.verticesList();
            std::sort(order.begin(), order.end());

            // Node labels (metadata)
            QJsonArray nodeLabels;
            for (int id : order)
                nodeLabels.append(g.vertexLabel(id));

            // Node custom attributes (metadata)
            QJsonArray nodeAttrs;
            for (int id : order)
                nodeAttrs.append(hashToJsonObjectSorted(g.vertexCustomAttributes(id)));

            // Canonical edge list (enabled edges only, current relation)
            QList<EdgeRow> edges;
            edges.reserve(g.edgesEnabled());

            // For undirected de-duplication: store each (min,max) once.
            QSet<qulonglong> seenUndir;

            for (int u : order)
            {
                GraphVertex *gv = g.vertexPtr(u);
                if (!gv)
                    continue;

                const QHash<int, qreal> out = gv->outEdgesEnabledHash(false /* allRelations */);
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

                    // Use canonical pair for label lookup.
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

            // Hash only the signature content (stable).
            const QByteArray canon = QJsonDocument(sig).toJson(QJsonDocument::Compact);
            sig["hash_sha256"] = QString::fromLatin1(sha256(canon).toHex());

            return sig;
        }

        // -------------------------------
        // Golden JSON builder + compare
        // -------------------------------

        static QJsonObject buildGoldenJsonV5Io(const CliConfig &cfg,
                                               const HeadlessLoadResult &load,
                                               Graph &g,
                                               const QJsonObject &sig,
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

            const int ties_graph = load.tiesGraph;
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

            QJsonObject metrics;
            metrics["density"] = d2s(g.graphDensity());
            root["metrics"] = metrics;

            QJsonObject timings;
            timings["load_ms"] = static_cast<qint64>(load.elapsedTime);
            timings["save_ms"] = static_cast<qint64>(saveMs);
            timings["reload_ms"] = static_cast<qint64>(reloadMs);
            timings["total_ms"] = static_cast<qint64>(totalMs);
            root["timings"] = timings;

            root["signature"] = sig;

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

        static int compareGoldenV5Io(const QJsonObject &expected, const QJsonObject &actual)
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

            // dataset
            const QJsonObject eDs = expected.value("dataset").toObject();
            const QJsonObject aDs = actual.value("dataset").toObject();
            ok &= cmpInt(eDs, aDs, "filetype", err);
            ok &= cmpStr(eDs, aDs, "name", err);

            // run
            const QJsonObject eRun = expected.value("run").toObject();
            const QJsonObject aRun = actual.value("run").toObject();
            ok &= cmpBool(eRun, aRun, "considerWeights", err);
            ok &= cmpBool(eRun, aRun, "inverseWeights", err);
            ok &= cmpBool(eRun, aRun, "dropIsolates", err);
            ok &= cmpStr(eRun, aRun, "operation", err);

            // counts
            const QJsonObject eC = expected.value("counts").toObject();
            const QJsonObject aC = actual.value("counts").toObject();
            ok &= cmpInt(eC, aC, "nodes", err);
            ok &= cmpInt(eC, aC, "links_sna", err);
            ok &= cmpInt(eC, aC, "ties_graph", err);

            // graph flags
            const QJsonObject eG = expected.value("graph").toObject();
            const QJsonObject aG = actual.value("graph").toObject();
            ok &= cmpBool(eG, aG, "directed", err);
            ok &= cmpBool(eG, aG, "weighted", err);

            // metrics
            const QJsonObject eM = expected.value("metrics").toObject();
            const QJsonObject aM = actual.value("metrics").toObject();
            ok &= cmpNumStrTol(eM, aM, "density", err, 0.0, 0.0);

            // signature
            const QJsonObject eS = expected.value("signature").toObject();
            const QJsonObject aS = actual.value("signature").toObject();

            ok &= cmpStr(eS, aS, "hash_sha256", err);

            ok &= cmpStrArray(eS.value("node_labels").toArray(),
                              aS.value("node_labels").toArray(),
                              err, "signature.node_labels");

            // strict JSON equality for attributes arrays
            if (eS.value("node_custom_attributes").toArray() != aS.value("node_custom_attributes").toArray())
            {
                err << "MISMATCH signature.node_custom_attributes differs\n";
                ok = false;
            }

            // strict edge list equality (u,v,w,label)
            if (eS.value("edge_list").toArray() != aS.value("edge_list").toArray())
            {
                err << "MISMATCH signature.edge_list differs\n";
                ok = false;
            }

            // roundtrip_result
            const QJsonObject eRt = expected.value("roundtrip_result").toObject();
            const QJsonObject aRt = actual.value("roundtrip_result").toObject();
            ok &= cmpBool(eRt, aRt, "performed", err);
            ok &= cmpBool(eRt, aRt, "equivalent", err);

            if (!ok)
                return 1;

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

        const QJsonObject sig1 = buildSignature(g);

        // These MUST live outside the if-block.
        bool performed = false;
        bool equivalent = true; // vacuously true if roundtrip not performed
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

            QElapsedTimer tSave;
            tSave.start();
            g.saveToFile(outPath, cfg.fileFormat, true /*saveEdgeWeights*/, false /*saveZeroWeightEdges*/);
            saveMs = tSave.elapsed();

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

            const QJsonObject sig2 = buildSignature(g2);

            equivalent = (sig1 == sig2);
            if (!equivalent)
            {
                QTextStream(stderr) << "MISMATCH_DETAIL:\n";
                printSigMismatchHint(sig1, sig2);
            }
        }

        // Build JSON (performed may be false now)
        const QJsonObject root = buildGoldenJsonV5Io(cfg, load, g, sig1,
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
            const int rc = compareGoldenV5Io(expected, root);
            if (rc != 0)
                return rc;
        }

        // Only fail on mismatch if roundtrip actually ran.
        if (performed && !equivalent)
        {
            QTextStream(stderr) << "MISMATCH: roundtrip signature differs (schema v5)\n";
            return 1;
        }

        return 0;
    }

} // namespace cli