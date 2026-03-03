#include "headless_graph_loader.h"

#include <QEventLoop>
#include <QThread>
#include <QMetaObject>
#include <QDebug>

#include "../graph/io/graph_parser_wiring.h"
#include "../graph/io/graph_parse_sink_graph.h"
#include "../graph.h"
#include "../parser.h"

/**
 * @brief Loads a graph from a given file, without any UI.
 * @returns a struct with load results (success, file type, node/link counts, etc.)
 */
HeadlessLoadResult loadGraphHeadless(
    Graph &graph,
    const QString &fileName,
    const QString &codecName,
    int fileFormat,
    const QString &delimiter,
    int sm_two_mode,
    bool sm_has_labels)
{
    HeadlessLoadResult out;
    out.ok = false;
    out.fileType = -1;
    out.totalNodes = 0;
    out.tiesGraph = 0;
    out.elapsedTime = 0;

    qDebug() << "[CLI] loadGraphHeadless() begin:" << fileName;

    // Same semantics as Graph::loadFile()
    graph.relationsClear();

    // Local parser + local thread (no Graph::file_parser involvement)
    auto *parser = new Parser();
    QThread parserThread;

    parser->moveToThread(&parserThread);

    SocNetV::IO::GraphParseSinkGraph sink(graph);
    parser->setParseSink(&sink);

    // We'll block until Graph emits signalGraphLoaded
    QEventLoop loop;
    bool done = false; // either one fires first (loaded OR finished), avoid double quit

    QObject::connect(&graph, &Graph::signalGraphLoaded,
                     &loop,
                     [&](const int &loadedFileType,
                         const QString &loadedFileName,
                         const QString &netName,
                         const int &totalNodes,
                         const int &tiesGraph,
                         const int & /*density*/,
                         const qint64 &elapsedTime,
                         const QString &message)
                     {
                         if (done)
                             return;
                         done = true;
                         out.fileType = loadedFileType;
                         out.fileName = loadedFileName;
                         out.netName = netName;
                         out.totalNodes = totalNodes;
                         // tiesGraph is emitted by Graph::graphFileLoaded() as edgesEnabled():
                         // logical ties count (E for undirected, A for directed).
                         out.tiesGraph = tiesGraph;
                         out.elapsedTime = elapsedTime;
                         out.message = message;
                         out.ok = (loadedFileType != FileType::UNRECOGNIZED);

                         qDebug() << "[CLI] signalGraphLoaded:"
                                  << "ok" << out.ok
                                  << "nodes" << out.totalNodes
                                  << "tiesGraph" << out.tiesGraph
                                  << "msg" << out.message;

                         loop.quit();
                     });

    // Also quit when parser finishes (belt-and-suspenders)
    QObject::connect(parser, &Parser::finished,
                     &loop,
                     [&](const QString &reason)
                     {
                         if (done)
                             return;
                         done = true;

                         qDebug() << "[CLI] Parser finished:" << reason;

                         // If graph didn't emit signalGraphLoaded for some reason, still quit.
                         if (out.fileType == -1)
                         {
                             out.ok = false;
                             out.message = reason;
                         }
                         loop.quit();
                     });

    parserThread.start();

    // --- Headless defaults (visual-only, do not affect distances/centralities) ---
    const int initVertexSize = 10;
    const QString initVertexColor = "#ffffff";
    const QString initVertexShape = "circle";
    const QString initVertexNumberColor = "#000000";
    const int initVertexNumberSize = 10;
    const QString initVertexLabelColor = "#000000";
    const int initVertexLabelSize = 10;
    const QString initEdgeColor = "#000000";
    const int canvasWidth = 700;
    const int canvasHeight = 600;

    // Queue Parser::load to run in parserThread *after* the event loop spins.
    QMetaObject::invokeMethod(
    parser,
    [parser,
     fileName,
     codecName,
     fileFormat,
     delimiter,
     sm_two_mode,
     sm_has_labels,
     initVertexSize,
     initVertexColor,
     initVertexShape,
     initVertexNumberColor,
     initVertexNumberSize,
     initVertexLabelColor,
     initVertexLabelSize,
     initEdgeColor,
     canvasWidth,
     canvasHeight]()
    {
        parser->load(fileName,
                     codecName,
                     initVertexSize,
                     initVertexColor,
                     initVertexShape,
                     initVertexNumberColor,
                     initVertexNumberSize,
                     initVertexLabelColor,
                     initVertexLabelSize,
                     initEdgeColor,
                     canvasWidth,
                     canvasHeight,
                     fileFormat,
                     delimiter,
                     sm_two_mode,
                     sm_has_labels);
    },
    Qt::QueuedConnection);

    qDebug() << "[CLI] entering loop.exec()";
    loop.exec();
    qDebug() << "[CLI] left loop.exec()";

    parserThread.quit();
    parserThread.wait();
    delete parser;
    parser = nullptr;

    qDebug() << "[CLI] loadGraphHeadless() end:" << out.ok;

    return out;
}
