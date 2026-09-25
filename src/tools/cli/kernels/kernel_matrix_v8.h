// SPDX-License-Identifier: GPL-3.0-or-later
// SocNetV - Social Network Visualizer
//
// Matrix golden coverage kernel (schema v8) for socnetv-cli.
// Dumps raw contents of every Matrix-producing Graph operation (adjacency, inverse,
// distances, shortest paths, similarity, dissimilarity, reachability, walks, clique
// co-membership) for direct regression coverage of Matrix::item()/setItem() indexing -
// see WS6.7. Also dumps AM's spectral-radius scalars (exact/bound/has_negative_entry, see
// Matrix::spectralRadiusExact()/spectralRadiusBound()/hasNegativeEntry()) - not a Matrix
// dump itself, but computed on AM right after it's built, so it lives here too.
#pragma once

#include "tools/cli/cli_common.h" // for cli::CliConfig

class Graph;
struct HeadlessLoadResult;

namespace cli {

int runKernelMatrixV8(const CliConfig &cfg,
                      const HeadlessLoadResult &load,
                      Graph &g);

} // namespace cli
