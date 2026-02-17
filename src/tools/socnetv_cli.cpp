#include <QCoreApplication>
#include <QCommandLineParser>
#include <QFileInfo>
#include <QTextStream>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QElapsedTimer>
#include <vector>
#include <algorithm>
#include <numeric>
#include <cmath>

#include "../graph.h"
#include "../graphvertex.h"
#include "headless_graph_loader.h"

static void printKV(const QString &k, double v)
{
    QTextStream(stdout) << k << "=" << QString::number(v, 'f', 3) << "\n";
}

static void printKV(const QString &k, const QString &v)
{
    QTextStream(stdout) << k << "=" << v << "\n";
}
static void printKV(const QString &k, int v)
{
    QTextStream(stdout) << k << "=" << v << "\n";
}
static void printKV(const QString &k, qint64 v)
{
    QTextStream(stdout) << k << "=" << v << "\n";
}

static QString d2s(double v)
{
    // Deterministic string for golden compare (avoid float parse/format differences).
    return QString::number(v, 'g', 17);
}

static bool writeJsonFile(const QString &path, const QJsonObject &obj, QString *err)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        if (err)
            *err = QString("Could not open for write: %1").arg(path);
        return false;
    }
    const QJsonDocument doc(obj);
    f.write(doc.toJson(QJsonDocument::Indented));
    return true;
}

static bool readJsonFile(const QString &path, QJsonObject *outObj, QString *err)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
    {
        if (err)
            *err = QString("Could not open for read: %1").arg(path);
        return false;
    }
    const QByteArray data = f.readAll();
    QJsonParseError pe;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &pe);
    if (doc.isNull() || !doc.isObject())
    {
        if (err)
            *err = QString("Invalid JSON (%1) in %2").arg(pe.errorString(), path);
        return false;
    }
    *outObj = doc.object();
    return true;
}

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
    reach["reachable_density"] = d2s(density); // as string, consistent with your float policy
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

// ---------- Compare helpers ----------

static bool cmpStr(const QJsonObject &e, const QJsonObject &a, const QString &k, QTextStream &err)
{
    const QString ev = e.value(k).toString();
    const QString av = a.value(k).toString();
    if (ev != av)
    {
        err << "MISMATCH " << k << " expected=" << ev << " got=" << av << "\n";
        return false;
    }
    return true;
}

static bool cmpInt(const QJsonObject &e, const QJsonObject &a, const QString &k, QTextStream &err)
{
    const int ev = e.value(k).toInt();
    const int av = a.value(k).toInt();
    if (ev != av)
    {
        err << "MISMATCH " << k << " expected=" << ev << " got=" << av << "\n";
        return false;
    }
    return true;
}

static bool cmpBool(const QJsonObject &e, const QJsonObject &a, const QString &k, QTextStream &err)
{
    const bool ev = e.value(k).toBool();
    const bool av = a.value(k).toBool();
    if (ev != av)
    {
        err << "MISMATCH " << k << " expected=" << (ev ? "true" : "false")
            << " got=" << (av ? "true" : "false") << "\n";
        return false;
    }
    return true;
}

static bool almostEqual(double a, double b, double rel = 1e-15, double abs = 0.0)
{
    // Treat NaN==NaN as equal for regression purposes
    if (std::isnan(a) && std::isnan(b))
        return true;

    // Treat +Inf==+Inf and -Inf==-Inf as equal
    if (std::isinf(a) || std::isinf(b))
        return a == b;

    const double diff = std::abs(a - b);
    if (diff <= abs)
        return true;
    const double scale = std::max(std::abs(a), std::abs(b));
    return diff <= rel * scale;
}

static bool cmpNumStrTol(const QJsonObject &e, const QJsonObject &a,
                         const QString &k, QTextStream &err,
                         double rel = 1e-15, double abs = 0.0)
{
    const QString es = e.value(k).toString();
    const QString as = a.value(k).toString();

    bool ok1 = false, ok2 = false;
    const double ev = es.toDouble(&ok1);
    const double av = as.toDouble(&ok2);

    if (!ok1 || !ok2)
    {
        err << "MISMATCH " << k << " non-numeric expected=" << es << " got=" << as << "\n";
        return false;
    }

    if (!almostEqual(ev, av, rel, abs))
    {
        err << "MISMATCH " << k << " expected=" << es << " got=" << as
            << " (diff=" << d2s(std::abs(ev - av)) << ")\n";
        return false;
    }

    return true;
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
        // same epsilon as graph-level
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

        // Centralities + standardized
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

    // Per-node vectors (only if computeCentralities==true in expected)
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

static bool cmpIntArray(const QJsonArray &e, const QJsonArray &a, QTextStream &err, const QString &what)
{
    if (e.size() != a.size())
    {
        err << "MISMATCH " << what << ".size expected=" << e.size() << " got=" << a.size() << "\n";
        return false;
    }
    for (int i = 0; i < e.size(); ++i)
    {
        const int ev = e.at(i).toInt();
        const int av = a.at(i).toInt();
        if (ev != av)
        {
            err << "MISMATCH " << what << "[" << i << "] expected=" << ev << " got=" << av << "\n";
            return false;
        }
    }
    return true;
}

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

    // Basic invariants (same checks as v1 where applicable)
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
            {
                ok = false;
            }
        }
    }

    if (!ok)
        return 1;
    err << "OK: baseline match\n";
    return 0;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QCommandLineParser cli;
    cli.addHelpOption();
    cli.addVersionOption();

    QCommandLineOption verboseOpt(QStringList() << "b" << "verbose", "Verbose debug output.");
    QCommandLineOption fileOpt(QStringList() << "i" << "input", "Input network file path.", "path");
    QCommandLineOption typeOpt(QStringList() << "f" << "format", "File type enum int (e.g. FileType::GRAPHML).", "int", "0");
    QCommandLineOption delimOpt(QStringList() << "d" << "delim", "Delimiter (for adjacency/edgelist).", "str", " ");
    QCommandLineOption twoModeOpt(QStringList() << "m" << "two-mode", "Two-mode int.", "int", "0");
    QCommandLineOption labelsOpt(QStringList() << "l" << "labels", "Adjacency has labels (0/1).", "int", "0");

    QCommandLineOption centralitiesOpt(QStringList() << "c" << "centralities", "Compute centralities (0/1).", "int", "1");
    QCommandLineOption weightsOpt(QStringList() << "w" << "weights", "Consider weights (0/1).", "int", "0");
    QCommandLineOption invWeightsOpt(QStringList() << "x" << "inverse-weights", "Inverse weights (0/1).", "int", "1");
    QCommandLineOption dropIsoOpt(QStringList() << "k" << "drop-isolates", "Drop isolates (0/1).", "int", "0");

    QCommandLineOption dumpJsonOpt(QStringList() << "j" << "dump-json",
                                   "Write golden output JSON to this path.", "path");
    QCommandLineOption compareJsonOpt(QStringList() << "p" << "compare-json",
                                      "Compare output against baseline JSON at path.", "path");

    QCommandLineOption benchOpt(QStringList() << "bench",
                                "Benchmark compute kernel: warmup once, then run N measured times. "
                                "Prints COMPUTE_MS_* stats. Disables JSON dump/compare.",
                                "N", "0");
    QCommandLineOption kernelOpt(QStringList() << "kernel",
                                 "Kernel to run: distance|reachability",
                                 "name", "distance");
    cli.addOption(kernelOpt);

    cli.addOption(verboseOpt);
    cli.addOption(fileOpt);
    cli.addOption(typeOpt);
    cli.addOption(delimOpt);
    cli.addOption(twoModeOpt);
    cli.addOption(labelsOpt);
    cli.addOption(centralitiesOpt);
    cli.addOption(weightsOpt);
    cli.addOption(invWeightsOpt);
    cli.addOption(dropIsoOpt);
    cli.addOption(dumpJsonOpt);
    cli.addOption(compareJsonOpt);
    cli.addOption(benchOpt);

    cli.process(app);

    const bool verbose = cli.isSet(verboseOpt);
    if (!verbose)
    {
        // Kill qDebug/qInfo output from Qt + your code (keeps warnings/criticals)
        qInstallMessageHandler([](QtMsgType type, const QMessageLogContext &, const QString &msg)
                               {
            if (type == QtWarningMsg || type == QtCriticalMsg || type == QtFatalMsg)
            {
                QTextStream(stderr) << msg << "\n";
            } });
    }

    const QString fileName = cli.value(fileOpt);
    if (fileName.isEmpty())
    {
        QTextStream(stderr) << "ERROR: --input is required\n";
        return 2;
    }

    const int fileFormat = cli.value(typeOpt).toInt();
    const QString delimiter = cli.value(delimOpt);
    const int sm_two_mode = cli.value(twoModeOpt).toInt();
    const bool sm_has_labels = (cli.value(labelsOpt).toInt() != 0);

    const bool computeCentralities = (cli.value(centralitiesOpt).toInt() != 0);
    const bool considerWeights = (cli.value(weightsOpt).toInt() != 0);
    const bool inverseWeights = (cli.value(invWeightsOpt).toInt() != 0);
    const bool dropIsolates = (cli.value(dropIsoOpt).toInt() != 0);

    const QString dumpJsonPath = cli.value(dumpJsonOpt);
    const QString compareJsonPath = cli.value(compareJsonOpt);

    const int benchRunsRaw = cli.value(benchOpt).toInt();
    const int benchRuns = (benchRunsRaw > 0) ? benchRunsRaw : 0;

    if (benchRuns > 0 && (!dumpJsonPath.isEmpty() || !compareJsonPath.isEmpty()))
    {
        QTextStream(stderr) << "ERROR: --bench cannot be combined with --dump-json or --compare-json\n";
        return 2;
    }

    const QString kernel = cli.value(kernelOpt).trimmed().toLower();
    if (benchRuns > 0 && kernel != "distance")
    {
        QTextStream(stderr) << "ERROR: --bench is only supported with --kernel distance\n";
        return 2;
    }

    Graph g;
    const QString codecName = "UTF-8";

    const auto load = loadGraphHeadless(g, fileName, codecName, fileFormat, delimiter, sm_two_mode, sm_has_labels);

    QTextStream(stderr) << "[CLI] after loadGraphHeadless()\n";

    printKV("OK", load.ok ? "1" : "0");
    printKV("FILE", QFileInfo(fileName).fileName());
    printKV("FILETYPE", load.fileType);
    printKV("N", load.totalNodes);

    // SNA: links/ties (A->B counts as 1). In undirected imports links may be 2 per edge depending on storage.
    printKV("LINKS_SNA", load.totalLinks);

    printKV("LOAD_MS", load.elapsedTime);
    printKV("LOAD_MSG", load.message);

    if (!load.ok)
        return 1;

    printKV("DIRECTED", g.isDirected() ? 1 : 0);
    printKV("WEIGHTED", g.isWeighted() ? 1 : 0);
    printKV("TIES_GRAPH", g.edgesEnabled());

    if (kernel == "reachability")
    {
        if (computeCentralities)
        {
            QTextStream(stderr) << "ERROR: --centralities is not applicable to --kernel reachability\n";
            return 2;
        }

        // Compute geodesic distances once (centralities must be false here)
        g.resetDistanceCentralityCacheFlags();
        QElapsedTimer t;
        t.start();
        g.graphDistancesGeodesic(false, considerWeights, inverseWeights, dropIsolates);
        const qint64 computeMs = t.elapsed();

        // Deterministic vertex order
        QList<int> order = g.verticesList(); // should already be deterministic
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
            fileName, fileFormat, load, g,
            considerWeights, inverseWeights, dropIsolates,
            order, matrix, ones);

        if (!dumpJsonPath.isEmpty())
        {
            QString err;
            if (!writeJsonFile(dumpJsonPath, actual, &err))
            {
                QTextStream(stderr) << "ERROR: " << err << "\n";
                return 2;
            }
            QTextStream(stderr) << "WROTE_JSON=" << dumpJsonPath << "\n";
        }

        if (!compareJsonPath.isEmpty())
        {
            QJsonObject expected;
            QString err;
            if (!readJsonFile(compareJsonPath, &expected, &err))
            {
                QTextStream(stderr) << "ERROR: " << err << "\n";
                return 2;
            }
            return compareGoldenV2Reachability(expected, actual);
        }

        return 0;
    }
    else if (kernel != "distance")
    {
        QTextStream(stderr) << "ERROR: unsupported --kernel: " << kernel << "\n";
        return 2;
    }

    // Ensure centralities are actually computed before we read per-node values.
    auto run_compute_once_ms = [&]() -> qint64
    {
        // Prevent early-return in DistanceEngine::compute()
        g.resetDistanceCentralityCacheFlags();

        QElapsedTimer t;
        t.start();

        // Canonical entry point (refactor target)
        g.graphDistancesGeodesic(computeCentralities,
                                 considerWeights,
                                 inverseWeights,
                                 dropIsolates);

        return t.elapsed();
    };

    if (benchRuns > 0)
    {
        // warmup (not measured)
        (void)run_compute_once_ms();

        std::vector<qint64> ms;
        ms.reserve(static_cast<size_t>(benchRuns));

        for (int r = 0; r < benchRuns; ++r)
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

        printKV("COMPUTE_RUNS", benchRuns);
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

    if (computeCentralities)
        printKV("PER_NODE", g.verticesList().size());

    const QJsonObject actual = buildGoldenJsonV1(
        fileName,
        fileFormat,
        load,
        g,
        computeCentralities,
        considerWeights,
        inverseWeights,
        dropIsolates,
        static_cast<double>(avgDist),
        diameter);

    if (!dumpJsonPath.isEmpty())
    {
        QString err;
        if (!writeJsonFile(dumpJsonPath, actual, &err))
        {
            QTextStream(stderr) << "ERROR: " << err << "\n";
            return 2;
        }
        QTextStream(stderr) << "WROTE_JSON=" << dumpJsonPath << "\n";
    }

    if (!compareJsonPath.isEmpty())
    {
        QJsonObject expected;
        QString err;
        if (!readJsonFile(compareJsonPath, &expected, &err))
        {
            QTextStream(stderr) << "ERROR: " << err << "\n";
            return 2;
        }
        return compareGoldenV1(expected, actual);
    }

    return 0;
}
