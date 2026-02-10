#pragma once

#include <QString>

class Graph;

struct HeadlessLoadResult
{
    int fileType = 0;
    QString fileName;
    QString netName;
    int totalNodes = 0;
    int totalLinks = 0;
    int edgeDirType = 0;
    qint64 elapsedTime = 0;
    QString message;
    bool ok = false;
};

HeadlessLoadResult loadGraphHeadless(
    Graph& graph,
    const QString& fileName,
    const QString& codecName,
    int fileFormat,
    const QString& delimiter,
    int sm_two_mode,
    bool sm_has_labels);
