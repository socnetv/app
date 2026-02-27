#pragma once

#include "tools/cli/cli_common.h"   // cli::CliConfig

class Graph;
struct HeadlessLoadResult;

namespace cli {

// Schema v5 — IO roundtrip regression kernel.
// SAME-FORMAT roundtrip (save->reload) via temp file.
// Strict equivalence is checked via a canonical signature built from the Graph model.
int runKernelIoRoundtripV5(const CliConfig &cfg,
                           const HeadlessLoadResult &load,
                           Graph &g);

} // namespace cli