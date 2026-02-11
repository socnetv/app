#include <QCoreApplication>
#include <QCommandLineParser>
#include <QFileInfo>
#include <QTextStream>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <cmath>
#include <algorithm>

#include "../graph.h"
#include "../graphvertex.h"
#include "headless_graph_loader.h"

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

    // Ensure centralities are actually computed before we read per-node values.
    // This is the canonical entry point for the refactor target.
    g.graphDistancesGeodesic(computeCentralities, considerWeights, inverseWeights, dropIsolates);

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
