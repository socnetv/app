#pragma once

#include <QString>
#include <QtGlobal>

class Graph;

/**
 * HeadlessLoadResult mirrors what Graph reports via signalGraphLoaded
 * after loading a dataset headlessly.
 *
 * Terminology (SocNetV canonical model, current relation only):
 * - tiesGraph: canonical tie count from the graph model, as returned by
 *              Graph::edgesEnabled().
 *              For undirected graphs this is |E| (unique edges).
 *              For directed graphs this is |A| (arcs).
 *
 * Important:
 * - This is NOT the Parser's raw totalLinks accumulator.
 * - Graph::graphFileLoaded() currently emits Graph::edgesEnabled() as the
 *   "totalLinks" argument in signalGraphLoaded (see issue #183 rationale).
 *
 * Derived reporting (CLI / golden JSON):
 * - SNA-style "links" where undirected edges are counted as 2 arcs must be
 *   derived as:
 *     links_sna = directed ? tiesGraph : 2 * tiesGraph
 */
struct HeadlessLoadResult
{
    bool ok = false;

    int fileType = -1;
    QString fileName;
    QString netName;

    int totalNodes = 0;
    int tiesGraph = 0;      // canonical ties == Graph::edgesEnabled()
    qint64 elapsedTime = 0; // ms
    QString message;
};

HeadlessLoadResult loadGraphHeadless(
    Graph &graph,
    const QString &fileName,
    const QString &codecName,
    int fileFormat,
    const QString &delimiter,
    int sm_two_mode,
    bool sm_has_labels);
