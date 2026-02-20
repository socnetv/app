#pragma once

#include <QJsonObject>
#include "tools/cli/cli_common.h"   // for cli::CliConfig

class Graph;
struct HeadlessLoadResult;

namespace cli {

// Runs schema v1 distance kernel.
// - If cfg.benchRuns > 0: prints COMPUTE_MS_* stats and returns 0.
// - Otherwise: runs once, prints metrics KVs, optionally dump/compare JSON.
// Return codes match existing behavior (0 ok, 1 mismatch, 2 usage/error).
int runKernelDistanceV1(const CliConfig &cfg,
                        const HeadlessLoadResult &load,
                        Graph &g);

} // namespace cli
