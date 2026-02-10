#include <QCoreApplication>
#include <QCommandLineParser>
#include <QFileInfo>
#include <QTextStream>
#include <QLoggingCategory>

#include "../graph.h"
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

    cli.process(app);

    const bool verbose = cli.isSet(verboseOpt);
    if (!verbose)
    {
           // Kill qDebug/qInfo output from Qt + your code (keeps warnings/criticals)
    qInstallMessageHandler([](QtMsgType type, const QMessageLogContext &, const QString &msg)
                           {
        if (type == QtWarningMsg || type == QtCriticalMsg || type == QtFatalMsg) {
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

    Graph g;
    const QString codecName = "UTF-8";

    const auto load = loadGraphHeadless(g, fileName, codecName, fileFormat, delimiter, sm_two_mode, sm_has_labels);
    QTextStream(stderr) << "[CLI] after loadGraphHeadless()\n";

    printKV("OK", load.ok ? "1" : "0");
    printKV("FILE", QFileInfo(fileName).fileName());
    printKV("FILETYPE", load.fileType);
    printKV("N", load.totalNodes);
    printKV("E", load.totalLinks);
    printKV("LOAD_MS", load.elapsedTime);
    printKV("LOAD_MSG", load.message);

    if (!load.ok)
        return 1;

    const qreal avgDist = g.graphDistanceGeodesicAverage(considerWeights, inverseWeights, dropIsolates);
    printKV("AVG_DIST", QString::number(avgDist, 'g', 12));

    printKV("DIAMETER", g.graphDiameter(considerWeights, inverseWeights));

    // Distances + optional centralities (uses your existing Graph API)

    printKV("DIRECTED", g.isDirected() ? 1 : 0);
    printKV("WEIGHTED", g.isWeighted() ? 1 : 0);

    qDebug() << "[CLI] Bye";

    return 0;
}
