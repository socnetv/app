#pragma once

#include "tools/cli/cli_common.h"

class Graph;
struct HeadlessLoadResult;

namespace cli
{

int runKernelClusteringV6(const CliConfig &cfg,
                          const HeadlessLoadResult &load,
                          Graph &g);

} // namespace cli