#pragma once

#include "tools/cli/cli_common.h" // for cli::CliConfig

class Graph;
struct HeadlessLoadResult;

namespace cli {

// Runs schema v2 reachability kernel.
// - Requires cfg.computeCentralities == false
// - Forbids cfg.benchRuns > 0 (bench is distance-only)
// - Uses g.graphDistancesGeodesic(false, ...) then builds 0/1 reachability matrix
// - Optionally dump/compare JSON
int runKernelReachabilityV2(const CliConfig &cfg,
                           const HeadlessLoadResult &load,
                           Graph &g);

} // namespace cli
