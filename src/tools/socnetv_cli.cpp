// SPDX-License-Identifier: GPL-3.0-or-later
// SocNetV - Social Network Visualizer
//
// socnetv-cli façade: argument parsing + dispatch only.
// All kernel logic lives in src/tools/cli/kernels/.

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QFileInfo>
#include <QTextStream>
#include <QMessageLogContext>
#include <QLoggingCategory>

#include "graph.h"
#include "tools/headless_graph_loader.h"

#include "tools/cli/cli_common.h"
#include "tools/cli/kernels/kernel_distance_v1.h"
#include "tools/cli/kernels/kernel_reachability_v2.h"
#include "tools/cli/kernels/kernel_walks_v3.h"
#include "tools/cli/kernels/kernel_prominence_v4.h"
#include "tools/cli/kernels/kernel_io_roundtrip_v5.h"
#include "tools/cli/kernels/kernel_clustering_v6.h"
#include "tools/cli/kernels/kernel_connectivity_v7.h"
#include "tools/cli/kernels/kernel_vertex_connectivity_v9.h"
#include "tools/cli/kernels/kernel_matrix_v8.h"

namespace
{
    // QString::toInt()/toDouble() silently return 0 on unparseable input (e.g. "-f graphml"
    // instead of "-f 1") instead of failing - the caller can't tell a genuine 0 from a typo,
    // and a value like "-f" happened to still "work" here since the loader falls back to
    // file-extension auto-detection, masking the mistake entirely. These wrappers use the
    // ok-flag overloads to reject bad input outright instead of silently defaulting to 0.
    bool parseIntArg(const QCommandLineParser &cli, const QCommandLineOption &opt, int &out)
    {
        bool ok = false;
        const QString raw = cli.value(opt);
        out = raw.toInt(&ok);
        if (!ok)
        {
            QTextStream(stderr) << "ERROR: --" << opt.names().last() << " expects an integer, got \""
                                 << raw << "\"\n";
        }
        return ok;
    }

    bool parseDoubleArg(const QCommandLineParser &cli, const QCommandLineOption &opt, qreal &out)
    {
        bool ok = false;
        const QString raw = cli.value(opt);
        out = raw.toDouble(&ok);
        if (!ok)
        {
            QTextStream(stderr) << "ERROR: --" << opt.names().last() << " expects a number, got \""
                                 << raw << "\"\n";
        }
        return ok;
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QCommandLineParser cli;
    cli.addHelpOption();
    cli.addVersionOption();

    QCommandLineOption verboseOpt(QStringList() << "b" << "verbose", "Verbose debug output.");
    QCommandLineOption strictOpt(QStringList() << "strict", "Fail on timing regressions (io_roundtrip kernel).");
    QCommandLineOption fileOpt(QStringList() << "i" << "input", "Input network file path.", "path");
    QCommandLineOption typeOpt(QStringList() << "f" << "format", "File type enum int.", "int", "0");
    QCommandLineOption delimOpt(QStringList() << "d" << "delim", "Delimiter.", "str", " ");
    QCommandLineOption twoModeOpt(QStringList() << "m" << "two-mode", "Two-mode int.", "int", "0");
    QCommandLineOption labelsOpt(QStringList() << "l" << "labels", "Adjacency has labels (0/1).", "int", "0");

    QCommandLineOption centralitiesOpt(QStringList() << "c" << "centralities", "Compute centralities (0/1).", "int", "1");
    QCommandLineOption weightsOpt(QStringList() << "w" << "weights", "Consider weights (0/1).", "int", "0");
    QCommandLineOption invWeightsOpt(QStringList() << "x" << "inverse-weights", "Inverse weights (0/1).", "int", "1");
    QCommandLineOption dropIsoOpt(QStringList() << "k" << "drop-isolates", "Drop isolates (0/1).", "int", "0");

    QCommandLineOption dumpJsonOpt(QStringList() << "j" << "dump-json", "Write JSON.", "path");
    QCommandLineOption compareJsonOpt(QStringList() << "p" << "compare-json", "Compare JSON.", "path");

    QCommandLineOption benchOpt(QStringList() << "bench",
                                "Benchmark compute kernel (distance only).",
                                "N", "0");

    QCommandLineOption kernelOpt(QStringList() << "kernel",
                                 "Kernel: distance|reachability|walks_matrix|prominence|io_roundtrip|clustering|connectivity|matrix|vertex_connectivity",
                                 "name", "distance");

    QCommandLineOption walksLenOpt(QStringList() << "walks-length",
                                   "Walks length K for walks_matrix (K>=1).",
                                   "K", "0");

    QCommandLineOption connTypeOpt(QStringList() << "connectivity-type",
                                   "Connectivity type for --kernel connectivity on directed graphs: weak|strong.",
                                   "type", "weak");

    QCommandLineOption connModeOpt(QStringList() << "conn-mode",
                                   "Mode for --kernel vertex_connectivity: local|global.",
                                   "mode", "global");
    QCommandLineOption connSourceOpt(QStringList() << "conn-source",
                                     "Source node number for --kernel vertex_connectivity --conn-mode local.",
                                     "int", "-1");
    QCommandLineOption connTargetOpt(QStringList() << "conn-target",
                                     "Target node number for --kernel vertex_connectivity --conn-mode local.",
                                     "int", "-1");

    QCommandLineOption katzAlphaOpt(QStringList() << "katz-alpha",
                                    "Attenuation factor alpha for --kernel prominence's Katz Centrality "
                                    "(must satisfy |alpha| < 1/lambda_max or the report will be all-zero). "
                                    "Omit to skip Katz Centrality entirely.",
                                    "alpha", "-1");

    QCommandLineOption bonacichAlphaOpt(QStringList() << "bonacich-alpha",
                                        "Overall scale factor alpha for --kernel prominence's Bonacich "
                                        "Power Centrality. Requires --bonacich-beta too. Omit to skip "
                                        "Bonacich Power Centrality entirely.",
                                        "alpha", "-1");

    QCommandLineOption bonacichBetaOpt(QStringList() << "bonacich-beta",
                                       "Attenuation factor beta for --kernel prominence's Bonacich Power "
                                       "Centrality (must satisfy |beta| < 1/lambda_max or the report will "
                                       "be all-zero). May be negative.",
                                       "beta", "0");

    QCommandLineOption similarityMeasureOpt(QStringList() << "similarity-measure",
                                            "Measure for --kernel matrix's similarity category: "
                                            "simple_matching|jaccard|pearson.",
                                            "measure", "simple_matching");

    QCommandLineOption similarityInputOpt(QStringList() << "similarity-input",
                                          "Input matrix for --kernel matrix's similarity category: "
                                          "adjacency|distances.",
                                          "input", "adjacency");

    cli.addOption(verboseOpt);
    cli.addOption(strictOpt);
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
    cli.addOption(kernelOpt);
    cli.addOption(walksLenOpt);
    cli.addOption(connTypeOpt);
    cli.addOption(connModeOpt);
    cli.addOption(connSourceOpt);
    cli.addOption(connTargetOpt);
    cli.addOption(katzAlphaOpt);
    cli.addOption(bonacichAlphaOpt);
    cli.addOption(bonacichBetaOpt);
    cli.addOption(similarityMeasureOpt);
    cli.addOption(similarityInputOpt);

    cli.process(app);

    cli::CliConfig cfg;

    cfg.verbose = cli.isSet(verboseOpt);
    cfg.strict = cli.isSet(strictOpt);

    if (!cfg.verbose)
    {
        // Kill qDebug/qInfo output from Qt + your code (keeps warnings/criticals)
        qInstallMessageHandler([](QtMsgType type, const QMessageLogContext &, const QString &msg)
                               {
        if (type == QtWarningMsg || type == QtCriticalMsg || type == QtFatalMsg)
        {
            QTextStream(stderr) << msg << "\n";
        } });
        // WS14: the message handler above only discards output AFTER a message has already
        // been formatted -- it does nothing for qCDebug(category)'s actual cost-saving
        // mechanism, which is skipping argument evaluation BEFORE formatting, gated by the
        // category's own enabled/disabled state. Without this, a converted category (e.g.
        // socnetv.engine) defaults to enabled here and every qCDebug() call still pays full
        // formatting cost, just to have the result thrown away by the handler above -- the
        // exact "format-then-discard" tax WS14 exists to eliminate. Explicitly disabling the
        // categories is what actually lets the category-gate short-circuit take effect.
        QLoggingCategory::setFilterRules("default.debug=false\n"
                                         "socnetv.*.debug=false");
    }
    cfg.inputPath = cli.value(fileOpt);
    cfg.delimiter = cli.value(delimOpt);

    int hasLabelsRaw = 0, computeCentralitiesRaw = 0, considerWeightsRaw = 0,
        inverseWeightsRaw = 0, dropIsolatesRaw = 0, benchRunsRaw = 0, walksLength = 0;

    bool argsOk = true;
    argsOk &= parseIntArg(cli, typeOpt, cfg.fileFormat);
    argsOk &= parseIntArg(cli, twoModeOpt, cfg.twoMode);
    argsOk &= parseIntArg(cli, labelsOpt, hasLabelsRaw);
    argsOk &= parseIntArg(cli, centralitiesOpt, computeCentralitiesRaw);
    argsOk &= parseIntArg(cli, weightsOpt, considerWeightsRaw);
    argsOk &= parseIntArg(cli, invWeightsOpt, inverseWeightsRaw);
    argsOk &= parseIntArg(cli, dropIsoOpt, dropIsolatesRaw);
    argsOk &= parseIntArg(cli, benchOpt, benchRunsRaw);
    argsOk &= parseIntArg(cli, walksLenOpt, walksLength);
    argsOk &= parseIntArg(cli, connSourceOpt, cfg.connSource);
    argsOk &= parseIntArg(cli, connTargetOpt, cfg.connTarget);
    argsOk &= parseDoubleArg(cli, katzAlphaOpt, cfg.katzAlpha);
    argsOk &= parseDoubleArg(cli, bonacichAlphaOpt, cfg.bonacichAlpha);
    argsOk &= parseDoubleArg(cli, bonacichBetaOpt, cfg.bonacichBeta);
    if (!argsOk)
    {
        return 2;
    }

    cfg.hasLabels = (hasLabelsRaw != 0);
    cfg.computeCentralities = (computeCentralitiesRaw != 0);
    cfg.considerWeights = (considerWeightsRaw != 0);
    cfg.inverseWeights = (inverseWeightsRaw != 0);
    cfg.dropIsolates = (dropIsolatesRaw != 0);

    cfg.dumpJsonPath = cli.value(dumpJsonOpt);
    cfg.compareJsonPath = cli.value(compareJsonOpt);

    cfg.benchRuns = (benchRunsRaw > 0) ? benchRunsRaw : 0;

    cfg.kernel = cli.value(kernelOpt).trimmed().toLower();
    cfg.connectivityType = cli.value(connTypeOpt).trimmed().toLower();
    cfg.connMode = cli.value(connModeOpt).trimmed().toLower();
    cfg.similarityMeasure = cli.value(similarityMeasureOpt).trimmed().toLower();
    cfg.similarityInput = cli.value(similarityInputOpt).trimmed().toLower();

    if (cfg.inputPath.isEmpty())
    {
        QTextStream(stderr) << "ERROR: --input is required\n";
        return 2;
    }

    if (cfg.benchRuns > 0 && cfg.kernel != "distance" && cfg.kernel != "prominence")
    {
        QTextStream(stderr) << "ERROR: --bench is only supported with --kernel distance or prominence\n";
        return 2;
    }

    if (cfg.similarityMeasure != "simple_matching" && cfg.similarityMeasure != "jaccard"
        && cfg.similarityMeasure != "pearson")
    {
        QTextStream(stderr) << "ERROR: --similarity-measure must be one of "
                                "simple_matching|jaccard|pearson\n";
        return 2;
    }

    if (cfg.similarityInput != "adjacency" && cfg.similarityInput != "distances")
    {
        QTextStream(stderr) << "ERROR: --similarity-input must be one of adjacency|distances\n";
        return 2;
    }

    Graph g;

    const auto load = loadGraphHeadless(
        g,
        cfg.inputPath,
        "UTF-8",
        cfg.fileFormat,
        cfg.delimiter,
        cfg.twoMode,
        cfg.hasLabels);

    cli::printKV("LOAD_OK", load.ok ? "1" : "0");
    cli::printKV("FILE", QFileInfo(cfg.inputPath).fileName());
    cli::printKV("FILETYPE", load.fileType);
    cli::printKV("N", load.totalNodes);
    cli::printKV("LOAD_MS", load.elapsedTime);
    cli::printKV("LOAD_MSG", load.message);

    if (!load.ok)
        return 1;

    // now safe to derive from graph state
    const int ties_graph = load.tiesGraph;
    const int links_sna = g.isDirected() ? ties_graph : (2 * ties_graph);

    cli::printKV("DIRECTED", g.isDirected() ? 1 : 0);
    cli::printKV("SYMMETRIC", g.isSymmetric() ? 1 : 0);
    cli::printKV("WEIGHTED", g.isWeighted() ? 1 : 0);
    cli::printKV("RELATIONS", g.relations());
    cli::printKV("TIES_GRAPH", ties_graph);
    cli::printKV("LINKS_SNA", links_sna);
    if (cfg.kernel == "io_roundtrip")
    {
        cli::printKV("KERNEL_DESC",
                     "io_roundtrip: load -> save(same-format) -> reload; compares per-relation signatures from the reloaded file");
    }

    if (cfg.kernel == "distance")
        return cli::runKernelDistanceV1(cfg, load, g);

    if (cfg.kernel == "reachability")
        return cli::runKernelReachabilityV2(cfg, load, g);

    if (cfg.kernel == "walks_matrix")
        return cli::runKernelWalksV3(cfg, load, g, walksLength);

    if (cfg.kernel == "prominence")
        return cli::runKernelProminenceV4(cfg, load, g);

    if (cfg.kernel == "io_roundtrip")
        return cli::runKernelIoRoundtripV5(cfg, load, g);

    if (cfg.kernel == "clustering")
        return runKernelClusteringV6(cfg, load, g);

    if (cfg.kernel == "connectivity")
        return cli::runKernelConnectivityV7(cfg, load, g);

    if (cfg.kernel == "matrix")
        return cli::runKernelMatrixV8(cfg, load, g);

    if (cfg.kernel == "vertex_connectivity")
        return cli::runKernelVertexConnectivityV9(cfg, load, g);

    QTextStream(stderr) << "ERROR: unsupported --kernel: " << cfg.kernel << "\n";
    return 2;
}
