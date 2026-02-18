#pragma once

#include <QJsonObject>

class Graph;
struct HeadlessLoadResult;

namespace cli {

struct CliConfig
{
    bool verbose = false;

    QString inputPath;
    int fileFormat = 0;
    QString delimiter;
    int twoMode = 0;
    bool hasLabels = false;

    bool computeCentralities = true;
    bool considerWeights = false;
    bool inverseWeights = true;
    bool dropIsolates = false;

    QString dumpJsonPath;
    QString compareJsonPath;

    int benchRuns = 0; // 0 = off
    QString kernel;    // "distance", etc.
};

// Runs schema v1 distance kernel.
// - If cfg.benchRuns > 0: prints COMPUTE_MS_* stats and returns 0.
// - Otherwise: runs once, prints metrics KVs, optionally dump/compare JSON.
// Return codes match existing behavior (0 ok, 1 mismatch, 2 usage/error).
int runKernelDistanceV1(const CliConfig &cfg,
                        const HeadlessLoadResult &load,
                        Graph &g);

} // namespace cli
