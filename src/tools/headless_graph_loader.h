#pragma once

#include <QString>
#include <QtGlobal>

class Graph;

/**
 * HeadlessLoadResult mirrors what Graph reports via signalGraphLoaded
 * after loading a dataset headlessly.
 *
 * Terminology:
 * - totalLinks: SNA links/ties as reported by the loader (GraphLoaded signal path).
 */
struct HeadlessLoadResult
{
    bool ok = false;

    int fileType = -1;
    QString fileName;
    QString netName;

    int totalNodes = 0;
    int totalLinks = 0;     // SNA "links" as reported by GraphLoaded
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
