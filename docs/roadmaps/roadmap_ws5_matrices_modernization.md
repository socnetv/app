# Matrices Modernization Roadmap (WS5)

## Goal

- Make the matrix subsystem genuinely faster — contiguous storage, flat-array APSP results.
- Isolate matrix creation and computation into coherent, testable types.
- Cancellation-aware algebra kernels.

## Status

✅ Complete. All seven milestones (A1-A7) shipped. `Matrix`'s internal storage and the
`QHash`-vs-`Matrix` APSP storage decision are documented in
[`README_DEVELOPER_NOTES.md`](../README_DEVELOPER_NOTES.md)'s "Matrix Storage" section, not here.
The cancellation flag-delivery mechanism these kernels depend on, and a busy-guard gap found while
verifying A6, are tracked in
[`roadmap_ws15_cancellation_progress_unification.md`](roadmap_ws15_cancellation_progress_unification.md).

## What WS5 Delivered

- **A1 — Inventory**: catalogued `Graph`'s named `Matrix` fields and every construction site
  scattered across `src/graph/`'s slice directories, prerequisite for A4.
- **A2 — APSP storage migration** (v3.7): `Graph::m_apspDist`/`m_apspSigma`
  (`QHash<relation_id, Matrix>`) replaced per-vertex `GraphVertex::m_distance`/`m_shortestPaths`
  QHash storage, migrated in three golden-verified steps. 15-17% faster at N=1,000, 9% faster at
  N=7,343 (`geom.net`), 1-3% faster end-to-end in the GUI.
- **A2.0 — Empirical validation** (v3.7): `matrix_storage_bench.cpp` compared `QHash` vs `Matrix`
  lookup storage at N=1,000/7,343 across three topologies — `Matrix` won lookup speed 22×-32× in
  every configuration and memory too for any topology with one dominant connected component.
- **A3 — Contiguous storage** (P1): replaced `MatrixRow*` (one heap allocation per row) with a
  flat `qreal*` buffer plus a precomputed row-pointer index. Allocation count 7,344→1 at
  `geom.net` scale; measured 38-64% faster construction and 14-19% faster cell-access lookup
  across all three A2.0 topologies (isolated benchmark, `--structure matrix` mode). End-to-end: no
  measurable change, expected since `DistanceEngine`'s BFS traversal dominates total work over
  matrix writes on that workload. Fixed three real bugs found during the accompanying doxygen pass
  (`operator*`'s wrong result shape, `swapRows()`/`multiplyRow()` using `rows()` instead of
  `cols()`, `productSym()` dead code). Also added `prominence --bench` support
  (`kernel_prominence_v4.cpp`, `PROM_BA500_M3` case) since A3's own end-to-end evidence needed a
  kernel exercising `WM`/`invM`, which `distance`-kernel benchmarks never touch.
- **A4 — Isolate construction into `matrices/`**: moved the four genuinely self-contained
  construction functions (`graphMatrixShortestPathsCreate`/`graphMatrixDistanceGeodesicCreate` →
  new `matrices/graph_matrix_distances.cpp`; `createMatrixReachability`/`graphWalksMatrixCreate` →
  new `matrices/graph_matrix_reachability.cpp`). Left `WM`/`invM` (`centralityInformation()`) and
  `CLQM` (`graphCliques()`/`graphCliqueAdd()`) in their algorithm slices — both inseparable from
  their surrounding computation, not standalone construction. Deleted the dead `sumM` field
  (declared and cleared on graph reset, never populated anywhere).
- **A5 — Cancellation-aware algebra kernels** (I1, I2): `Matrix::inverse()`/`powerIteration()`
  gained an optional `cancelCheck` parameter, checked once per outer-loop iteration, wired at
  `createMatrixAdjacencyInverse()`/`centralityInformation()`/`centralityEigenvector()`. Live
  testing found the flag-delivery mechanism itself broken (a Graph/MainWindow wiring issue, not
  `Matrix`) — fixed in WS15 P1.
- **A6 — Cancellation in `distancesMatrix()`/`similarityMatrix()`/`pearsonCorrelationCoefficients()`**
  (I4 — corrected from `writeMatrix()`'s dead `MATRIX_DISTANCES_*` switch-cases to the real live
  call path once traced): same `cancelCheck` convention as A5, wired at their three `Graph::`
  callers in `similarity/graph_similarity_matrices.cpp` and post-call checks in their three
  `write*` callers. Found and fixed a related crash during live-testing: the toolbox dock panel's
  comboboxes (plain `QComboBox`es, not `QAction`s) were never covered by
  `MainWindow::setAppBusy()`'s disable sweep, letting a second report race a still-running one on
  shared `Graph::` matrix members — reproduced as a `SIGSEGV` in `Matrix::item()`. Fixed by
  disabling `leftPanel` (the toolbox's container) in `setAppBusy()` alongside
  `menuBar()`/`toolBar`/`graphicsWidget`.
- **A7 — Golden coverage for matrix operations**: delivered via
  [`roadmap_ws6_testing_ci_regression.md`](roadmap_ws6_testing_ci_regression.md)'s WS6.7
  (`kernel_matrix_v8`) — full coverage audit, design decisions, and empirical XSM/CLQM timing
  findings live in that doc.

## What Remains Open

- **C1 — `createMatrixAdjacencyInverse()` reports `invertible=true` for a mathematically singular
  matrix** ([#269](https://github.com/socnetv/app/issues/269), found while building WS6.7's
  `kernel_matrix_v8`). Its singularity check inspects the LU-decomposition *result* for nonzero
  off-diagonal entries rather than pivot magnitude *during* decomposition — a singular input
  produces numerical garbage (confirmed on `TinyPath_N3_E2`: `1e+20`/`-1e+20` entries) that still
  passes the check. Information Centrality is the real downstream consumer at risk. Proposed fix
  (check pivot magnitude in `ludcmp()` against a relative, not absolute, tolerance) not
  implemented; natural fit alongside `Matrix::inverse()`'s internals whenever picked up.
- The dead `MATRIX_DISTANCES_EUCLIDEAN`/`HAMMING`/`JACCARD`/`MANHATTAN`/`CHEBYSHEV` switch-cases in
  `writeMatrix()` (`graph/reporting/graph_reports.cpp:5663-5919`) — unreachable, no caller passes
  those enum values. Found while scoping A6; not removed.
- **Perf baseline recording for A3/A6-touched kernels** — deferred to the v3.7 tag per the
  project's clean-tagged-release rule (baselines are never recorded against `develop` HEAD). An
  interim, non-committed reading exists for `PROM_BA500_M3` (this machine, current `develop`:
  median 40ms) for reference until the real recording happens at the tag.
- `README__RELEASE_PROCEDURE.md` has no step for (re-)recording perf baselines at release time —
  found while checking the above; worth adding.
