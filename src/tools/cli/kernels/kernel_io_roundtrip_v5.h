#pragma once

#include "tools/cli/cli_common.h"   // cli::CliConfig

class Graph;
struct HeadlessLoadResult;

namespace cli {

// Schema v5 — IO roundtrip regression kernel (multirelational-aware).
// - Always reports relations count + per-relation signatures.
// - SAME-FORMAT roundtrip if exporter exists; otherwise performed=false.
// - If roundtrip performed: strict equality across relations_bundle required.
int runKernelIoRoundtripV5(const CliConfig &cfg,
                           const HeadlessLoadResult &load,
                           Graph &g);

} // namespace cli