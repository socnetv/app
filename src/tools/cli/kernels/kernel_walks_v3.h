#pragma once

#include "tools/cli/cli_common.h" // for cli::CliConfig

class Graph;
struct HeadlessLoadResult;

namespace cli {

// Runs schema v3 walks_matrix kernel (exact length K).
// - Requires cfg.computeCentralities == false
// - Forbids cfg.benchRuns > 0 (bench is distance-only)
// - Requires walksLength >= 1
// - Uses Graph::walksBetween(src,dst,K) to build NxN matrix of integer strings
// - Optionally dump/compare JSON
int runKernelWalksV3(const CliConfig &cfg,
                     const HeadlessLoadResult &load,
                     Graph &g,
                     int walksLength);

} // namespace cli
