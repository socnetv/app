// SPDX-License-Identifier: GPL-3.0-or-later
// SocNetV - Social Network Visualizer
//
// Matrix golden coverage kernel (schema v8) for socnetv-cli.
// See kernel_matrix_v8.h and roadmap_ws6_testing_ci_regression.md's WS6.7 section for
// the design rationale: existing kernels only ever check downstream results (centrality
// scores, distance values, clique counts), never a Matrix's actual contents, so a subtle
// Matrix::item()/setItem() indexing bug could slip through untested. This kernel dumps raw
// matrix contents for every Matrix-producing Graph operation instead.

#include "kernel_matrix_v8.h"

#include "graph.h"
#include "tools/cli/cli_common.h"
#include "tools/headless_graph_loader.h"

#include <QElapsedTimer>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QTextStream>

#include <algorithm>

namespace cli {

// Networks above this size skip the total-walks (XSM) category: computing it (summing
// matrix powers up to N-1) measured ~9.2 min (553,940 ms) at N=500 on
// Benchmark_BA_Directed_N500_m3 (MacBook Pro M5, 24GB RAM, unoptimized/default build
// type - not a Release number, and not tied to this specific machine either; the
// point is minutes, not the exact figure). Clique co-membership (CLQM) needs no such
// gate - measured 6 ms on the same run - so it's always computed.
static const int kTotalWalksSkipThreshold = 50;

// Networks above this size are dumped as a compact summary (row/col sums, trace, a
// handful of sampled cells) instead of the full N*N grid - a full-grid JSON dump is
// impractical past a few dozen nodes (WS5 A2.0 measured ~461 MB for one Matrix at
// N=7343).
static const int kFullGridSizeLimit = 20;

// ---- per-matrix dump/compare ----

// Serializes one Matrix's contents: the full N*N grid if fullGrid is true, otherwise a
// compact summary (row/col sums, trace, five sampled cells).
static QJsonObject dumpMatrixJson(Matrix &m, bool fullGrid)
{
    QJsonObject o;
    const int rows = m.rows();
    const int cols = m.cols();
    o["dump_mode"] = fullGrid ? "full" : "summary";
    o["rows"] = rows;
    o["cols"] = cols;

    if (fullGrid)
    {
        QJsonArray data;
        for (int r = 0; r < rows; ++r)
        {
            QJsonArray row;
            for (int c = 0; c < cols; ++c)
                row.append(d2s(m.item(r, c)));
            data.append(row);
        }
        o["data"] = data;
        return o;
    }

    QJsonArray rowSums, colSums;
    for (int r = 0; r < rows; ++r)
    {
        qreal sum = 0;
        for (int c = 0; c < cols; ++c)
            sum += m.item(r, c);
        rowSums.append(d2s(sum));
    }
    for (int c = 0; c < cols; ++c)
    {
        qreal sum = 0;
        for (int r = 0; r < rows; ++r)
            sum += m.item(r, c);
        colSums.append(d2s(sum));
    }
    o["row_sums"] = rowSums;
    o["col_sums"] = colSums;

    qreal trace = 0;
    const int diag = std::min(rows, cols);
    for (int i = 0; i < diag; ++i)
        trace += m.item(i, i);
    o["trace"] = d2s(trace);

    QJsonObject samples;
    auto sample = [&](int r, int c) {
        if (r < 0 || c < 0 || r >= rows || c >= cols)
            return;
        samples[QString("%1,%2").arg(r).arg(c)] = d2s(m.item(r, c));
    };
    sample(0, 0);
    sample(0, cols - 1);
    sample(rows - 1, 0);
    sample(rows - 1, cols - 1);
    sample(rows / 2, cols / 2);
    o["sample_cells"] = samples;

    return o;
}

// Compares two JSON arrays of d2s()-formatted numeric strings element-by-element,
// with the same tolerance almostEqual() uses elsewhere in the CLI harness.
static bool cmpNumStrArray(const QJsonArray &e, const QJsonArray &a, QTextStream &err, const QString &what)
{
    if (e.size() != a.size())
    {
        err << "MISMATCH " << what << ".size expected=" << e.size() << " got=" << a.size() << "\n";
        return false;
    }
    bool ok = true;
    for (int i = 0; i < e.size(); ++i)
    {
        bool ok1 = false, ok2 = false;
        const double ev = e.at(i).toString().toDouble(&ok1);
        const double av = a.at(i).toString().toDouble(&ok2);
        if (!ok1 || !ok2 || !almostEqual(ev, av))
        {
            err << "MISMATCH " << what << "[" << i << "] expected=" << e.at(i).toString()
                << " got=" << a.at(i).toString() << "\n";
            ok = false;
        }
    }
    return ok;
}

// Compares one category's dumpMatrixJson() output against its baseline, dispatching to
// either a full-grid or a summary-tier comparison depending on the baseline's dump_mode.
static bool compareMatrixJson(const QJsonObject &e, const QJsonObject &a, QTextStream &err, const QString &what)
{
    bool ok = true;
    ok &= cmpStr(e, a, "dump_mode", err);
    ok &= cmpInt(e, a, "rows", err);
    ok &= cmpInt(e, a, "cols", err);

    if (e.value("dump_mode").toString() == "full")
    {
        const QJsonArray eData = e.value("data").toArray();
        const QJsonArray aData = a.value("data").toArray();
        if (eData.size() != aData.size())
        {
            err << "MISMATCH " << what << ".data.rows expected=" << eData.size()
                << " got=" << aData.size() << "\n";
            return false;
        }
        for (int r = 0; r < eData.size(); ++r)
            ok &= cmpNumStrArray(eData.at(r).toArray(), aData.at(r).toArray(), err,
                                 QString("%1.data[%2]").arg(what).arg(r));
        return ok;
    }

    ok &= cmpNumStrArray(e.value("row_sums").toArray(), a.value("row_sums").toArray(), err, what + ".row_sums");
    ok &= cmpNumStrArray(e.value("col_sums").toArray(), a.value("col_sums").toArray(), err, what + ".col_sums");
    ok &= cmpNumStrTol(e, a, "trace", err);

    const QJsonObject eSamples = e.value("sample_cells").toObject();
    const QJsonObject aSamples = a.value("sample_cells").toObject();
    const QStringList keys = eSamples.keys();
    if (keys.size() != aSamples.keys().size())
    {
        err << "MISMATCH " << what << ".sample_cells.size expected=" << keys.size()
            << " got=" << aSamples.keys().size() << "\n";
        ok = false;
    }
    for (const QString &k : keys)
    {
        if (!aSamples.contains(k))
        {
            err << "MISMATCH " << what << ".sample_cells missing key=" << k << "\n";
            ok = false;
            continue;
        }
        ok &= cmpNumStrTol(eSamples, aSamples, k, err);
    }
    return ok;
}

// ---- schema v8 builder ----

// Assembles the full schema-v8 JSON document: dataset/counts/graph/run metadata plus the
// already-built "matrices" object (one entry per category, see runKernelMatrixV8()).
static QJsonObject buildGoldenJsonV8(
    const QString &inputPath,
    int fileFormat,
    const HeadlessLoadResult &load,
    Graph &g,
    bool considerWeights,
    bool inverseWeights,
    bool dropIsolates,
    const QJsonObject &matrices)
{
    QJsonObject root;
    root["schema_version"] = 8;
    root["kernel"] = "matrix";

    QJsonObject dataset;
    dataset["path"] = inputPath;
    dataset["name"] = QFileInfo(inputPath).fileName();
    dataset["filetype"] = fileFormat;
    root["dataset"] = dataset;

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

    QJsonObject run;
    run["considerWeights"] = considerWeights;
    run["inverseWeights"] = inverseWeights;
    run["dropIsolates"] = dropIsolates;
    root["run"] = run;

    root["matrices"] = matrices;

    QJsonObject loadReport;
    loadReport["ok"] = load.ok;
    loadReport["fileType_signal"] = load.fileType;
    loadReport["load_ms"] = static_cast<qint64>(load.elapsedTime);
    loadReport["load_msg"] = load.message;
    loadReport["net_name"] = load.netName;
    root["load_report"] = loadReport;

    return root;
}

// ---- schema v8 comparator ----

// Compares an actual run's schema-v8 JSON against a committed baseline. Returns 0 on
// match, 1 on a data mismatch, 2 on a structural error (unsupported schema, baseline
// missing a category it must have). total_walks is the only optional category, gated
// by kTotalWalksSkipThreshold.
static int compareGoldenV8(const QJsonObject &expected, const QJsonObject &actual)
{
    QTextStream err(stderr);

    if (expected.value("schema_version").toInt() != 8 || actual.value("schema_version").toInt() != 8)
    {
        err << "ERROR: schema_version mismatch or unsupported\n";
        return 2;
    }

    bool ok = true;

    ok &= cmpInt(expected.value("dataset").toObject(), actual.value("dataset").toObject(), "filetype", err);
    ok &= cmpStr(expected.value("dataset").toObject(), actual.value("dataset").toObject(), "name", err);
    ok &= cmpInt(expected.value("counts").toObject(), actual.value("counts").toObject(), "nodes", err);
    ok &= cmpBool(expected.value("graph").toObject(), actual.value("graph").toObject(), "directed", err);
    ok &= cmpBool(expected.value("graph").toObject(), actual.value("graph").toObject(), "weighted", err);

    const QJsonObject eM = expected.value("matrices").toObject();
    const QJsonObject aM = actual.value("matrices").toObject();

    static const QStringList kAlwaysPresent = {
        "adjacency", "adjacency_inverse", "distances", "similarity", "reachability", "walks",
        "clique_comembership"
    };
    for (const QString &cat : kAlwaysPresent)
    {
        if (!eM.contains(cat))
        {
            err << "ERROR: expected baseline missing category=" << cat << "\n";
            return 2;
        }
        if (!aM.contains(cat))
        {
            err << "MISMATCH missing actual category=" << cat << "\n";
            ok = false;
            continue;
        }
        ok &= compareMatrixJson(eM.value(cat).toObject(), aM.value(cat).toObject(), err, cat);
    }

    ok &= cmpBool(eM.value("adjacency_inverse").toObject(), aM.value("adjacency_inverse").toObject(),
                  "invertible", err);
    ok &= cmpInt(eM.value("walks").toObject(), aM.value("walks").toObject(), "length", err);
    ok &= cmpStr(eM.value("similarity").toObject(), aM.value("similarity").toObject(), "metric", err);

    // total_walks only exists for small fixtures - see kTotalWalksSkipThreshold.
    for (const QString &cat : {QStringLiteral("total_walks")})
    {
        const bool eHas = eM.contains(cat);
        const bool aHas = aM.contains(cat);
        if (eHas != aHas)
        {
            err << "MISMATCH category presence for " << cat << " expected_present=" << eHas
                << " actual_present=" << aHas << "\n";
            ok = false;
            continue;
        }
        if (eHas)
            ok &= compareMatrixJson(eM.value(cat).toObject(), aM.value(cat).toObject(), err, cat);
    }

    if (!ok)
        return 1;

    err << "OK: baseline match\n";
    return 0;
}

// ---- exported runner ----

// Builds every Matrix-producing category (adjacency, inverse, distances, similarity,
// reachability, walks, total walks, clique co-membership) in turn, dumping each one's
// contents right after it's constructed, then writes/compares the resulting JSON per
// --dump-json/--compare-json.
int runKernelMatrixV8(const CliConfig &cfg,
                      const HeadlessLoadResult &load,
                      Graph &g)
{
    if (cfg.computeCentralities)
    {
        QTextStream(stderr) << "ERROR: --centralities is not applicable to --kernel matrix\n";
        return 2;
    }
    if (cfg.benchRuns > 0)
    {
        QTextStream(stderr) << "ERROR: --bench is only supported with --kernel distance\n";
        return 2;
    }

    g.resetDistanceCentralityCacheFlags();

    const int n = g.vertices();
    const bool fullGrid = (n <= kFullGridSizeLimit);
    const bool includeTotalWalks = (n <= kTotalWalksSkipThreshold);

    QElapsedTimer t;
    t.start();

    QJsonObject matrices;

    // Adjacency - dumped immediately, before createMatrixAdjacencyInverse() runs, since
    // that call internally rebuilds AM with dropIsolates forced true (see
    // graph_matrix_adjacency.cpp) - capturing the JSON right after each construction
    // call, rather than re-reading Graph state at the very end, avoids depending on
    // later steps' internal side effects on shared fields like AM.
    g.createMatrixAdjacency();
    matrices["adjacency"] = dumpMatrixJson(g.matrixAdjacency(), fullGrid);

    const bool invertible = g.createMatrixAdjacencyInverse("lu");
    QJsonObject inv = dumpMatrixJson(g.matrixAdjacencyInverse(), fullGrid);
    inv["invertible"] = invertible;
    matrices["adjacency_inverse"] = inv;

    g.graphMatrixDistanceGeodesicCreate(cfg.considerWeights, cfg.inverseWeights, cfg.dropIsolates);
    matrices["distances"] = dumpMatrixJson(g.matrixDistances(), fullGrid);

    // Similarity needs a fresh adjacency input - createMatrixAdjacencyInverse() already
    // overwrote AM with its own dropIsolates=true policy, so rebuild once more here.
    g.createMatrixAdjacency();
    Matrix similarity;
    // Fix #279: which measure runs is selectable via --similarity-measure (default
    // simple_matching, unchanged from before) so the NaN-guard fix on Jaccard's and
    // Pearson's degenerate (empty-sample, diagonal=false) path can each get their own
    // golden baseline instead of only ever exercising simple_matching.
    // --similarity-input selects which matrix feeds similarityMatrix(): the default
    // adjacency (AM, never contains RAND_MAX) or the geodesic distances matrix just
    // computed above (DM, which does for unreachable pairs) - needed to cover Jaccard's
    // RAND_MAX-exclusion fix, whose effect is invisible on AM.
    Matrix &similarityInput = (cfg.similarityInput == "distances") ? g.matrixDistances()
                                                                   : g.matrixAdjacency();
    if (cfg.similarityMeasure == "pearson")
    {
        g.createMatrixSimilarityPearson(similarityInput, similarity, "Rows", false);
    }
    else
    {
        const int measure = (cfg.similarityMeasure == "jaccard") ? METRIC_JACCARD_INDEX
                                                                  : METRIC_SIMPLE_MATCHING;
        g.createMatrixSimilarityMatching(similarityInput, similarity, measure, "Rows", false, false);
    }
    QJsonObject sim = dumpMatrixJson(similarity, fullGrid);
    sim["metric"] = cfg.similarityMeasure;
    sim["input"] = cfg.similarityInput;
    matrices["similarity"] = sim;

    g.createMatrixReachability();
    matrices["reachability"] = dumpMatrixJson(g.matrixReachability(), fullGrid);

    const int walksN = g.vertices();
    g.graphWalksMatrixCreate(walksN, /*length=*/2,
                             /*dropIsolates=*/false, /*considerWeights=*/false,
                             /*inverseWeights=*/false, /*symmetrize=*/false);
    QJsonObject walks = dumpMatrixJson(g.matrixWalks(), fullGrid);
    walks["length"] = 2;
    matrices["walks"] = walks;

    if (includeTotalWalks)
    {
        g.graphWalksMatrixCreate(walksN, /*length=*/0, false, false, false, false);
        matrices["total_walks"] = dumpMatrixJson(g.matrixTotalWalks(), fullGrid);
    }

    // Unlike total walks, clique co-membership stays cheap at scale (Bron-Kerbosch on a
    // sparse graph) - see the kTotalWalksSkipThreshold comment above for the measurement
    // that justified computing this one unconditionally.
    g.graphCliques();
    matrices["clique_comembership"] = dumpMatrixJson(g.matrixCliqueCoMembership(), fullGrid);

    const qint64 computeMs = t.elapsed();

    printKV("COMPUTE_MS", computeMs);
    printKV("MATRIX_N", n);
    printKV("MATRIX_FULL_GRID", fullGrid ? 1 : 0);
    printKV("MATRIX_TOTAL_WALKS_INCLUDED", includeTotalWalks ? 1 : 0);

    const QJsonObject actual = buildGoldenJsonV8(
        cfg.inputPath, cfg.fileFormat, load, g,
        cfg.considerWeights, cfg.inverseWeights, cfg.dropIsolates, matrices);

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
        return compareGoldenV8(expected, actual);
    }

    return 0;
}

} // namespace cli
