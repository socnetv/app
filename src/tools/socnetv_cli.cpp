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

#include "graph.h"
#include "tools/headless_graph_loader.h"

#include "tools/cli/cli_common.h"
#include "tools/cli/kernels/kernel_distance_v1.h"
#include "tools/cli/kernels/kernel_reachability_v2.h"
#include "tools/cli/kernels/kernel_walks_v3.h"
#include "tools/cli/kernels/kernel_prominence_v4.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QCommandLineParser cli;
    cli.addHelpOption();
    cli.addVersionOption();

    QCommandLineOption verboseOpt(QStringList() << "b" << "verbose", "Verbose debug output.");
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
                                 "Kernel: distance|reachability|walks_matrix|prominence",
                                 "name", "distance");

    QCommandLineOption walksLenOpt(QStringList() << "walks-length",
                                   "Walks length K for walks_matrix (K>=1).",
                                   "K", "0");

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
    cli.addOption(kernelOpt);
    cli.addOption(walksLenOpt);

    cli.process(app);

    cli::CliConfig cfg;

    cfg.verbose = cli.isSet(verboseOpt);
    if (!cfg.verbose)
    {
        // Kill qDebug/qInfo output from Qt + your code (keeps warnings/criticals)
        qInstallMessageHandler([](QtMsgType type, const QMessageLogContext &, const QString &msg)
                               {
        if (type == QtWarningMsg || type == QtCriticalMsg || type == QtFatalMsg)
        {
            QTextStream(stderr) << msg << "\n";
        } });
    }
    cfg.inputPath = cli.value(fileOpt);
    cfg.fileFormat = cli.value(typeOpt).toInt();
    cfg.delimiter = cli.value(delimOpt);
    cfg.twoMode = cli.value(twoModeOpt).toInt();
    cfg.hasLabels = (cli.value(labelsOpt).toInt() != 0);

    cfg.computeCentralities = (cli.value(centralitiesOpt).toInt() != 0);
    cfg.considerWeights = (cli.value(weightsOpt).toInt() != 0);
    cfg.inverseWeights = (cli.value(invWeightsOpt).toInt() != 0);
    cfg.dropIsolates = (cli.value(dropIsoOpt).toInt() != 0);

    cfg.dumpJsonPath = cli.value(dumpJsonOpt);
    cfg.compareJsonPath = cli.value(compareJsonOpt);

    const int benchRunsRaw = cli.value(benchOpt).toInt();
    cfg.benchRuns = (benchRunsRaw > 0) ? benchRunsRaw : 0;

    cfg.kernel = cli.value(kernelOpt).trimmed().toLower();
    const int walksLength = cli.value(walksLenOpt).toInt();

    if (cfg.inputPath.isEmpty())
    {
        QTextStream(stderr) << "ERROR: --input is required\n";
        return 2;
    }

    if (cfg.benchRuns > 0 && cfg.kernel != "distance")
    {
        QTextStream(stderr) << "ERROR: --bench is only supported with --kernel distance\n";
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

    cli::printKV("OK", load.ok ? "1" : "0");
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
    cli::printKV("WEIGHTED", g.isWeighted() ? 1 : 0);
    cli::printKV("TIES_GRAPH", ties_graph);
    cli::printKV("LINKS_SNA", links_sna);

    if (cfg.kernel == "distance")
        return cli::runKernelDistanceV1(cfg, load, g);

    if (cfg.kernel == "reachability")
        return cli::runKernelReachabilityV2(cfg, load, g);

    if (cfg.kernel == "walks_matrix")
        return cli::runKernelWalksV3(cfg, load, g, walksLength);
    
    if (cfg.kernel == "prominence")
        return cli::runKernelProminenceV4(cfg, load, g);

    QTextStream(stderr) << "ERROR: unsupported --kernel: " << cfg.kernel << "\n";
    return 2;
}
