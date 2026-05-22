// SPDX-License-Identifier: GPL-3.0-or-later
// SocNetV - Social Network Visualizer
//
// Connectivity kernel (schema v7) for socnetv-cli.
// Reports connected/disconnected status and weakly connected component count.
#pragma once

#include "tools/cli/cli_common.h"

class Graph;
struct HeadlessLoadResult;

namespace cli
{

int runKernelConnectivityV7(const CliConfig &cfg,
                            const HeadlessLoadResult &load,
                            Graph &g);

} // namespace cli
