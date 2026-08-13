// SPDX-License-Identifier: GPL-3.0-or-later
// SocNetV - Social Network Visualizer
//
// Vertex connectivity kernel (schema v9) for socnetv-cli.
// Local (--conn-mode local --conn-source S --conn-target T) or global (--conn-mode global)
// vertex connectivity via Graph::graphNodeConnectivity() / Graph::graphConnectivity().
#pragma once

#include "tools/cli/cli_common.h"

class Graph;
struct HeadlessLoadResult;

namespace cli
{

int runKernelVertexConnectivityV9(const CliConfig &cfg,
                                  const HeadlessLoadResult &load,
                                  Graph &g);

} // namespace cli
