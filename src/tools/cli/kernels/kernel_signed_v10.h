// SPDX-License-Identifier: GPL-3.0-or-later
// SocNetV - Social Network Visualizer
//
// Signed-network analysis kernel (schema v10) for socnetv-cli.
// Reports Johnson's-algorithm potentials and negative-cycle detection.
#pragma once

#include "tools/cli/cli_common.h"

class Graph;
struct HeadlessLoadResult;

namespace cli
{

int runKernelSignedV10(const CliConfig &cfg,
                       const HeadlessLoadResult &load,
                       Graph &g);

} // namespace cli
