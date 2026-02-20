#pragma once

#include <QJsonObject>
#include "tools/cli/cli_common.h" // for cli::CliConfig

class Graph;
struct HeadlessLoadResult;

namespace cli {

// Runs schema v4 prominence kernel.
// Always computes prominence indices (ignores cfg.computeCentralities toggle).
// Optional dump/compare JSON.
// Return codes:
//   0 = ok
//   1 = mismatch
//   2 = usage/error
int runKernelProminenceV4(const CliConfig &cfg,
                          const HeadlessLoadResult &load,
                          Graph &g);

} // namespace cli