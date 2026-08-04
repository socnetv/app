# Matrices Modernization Roadmap (WS5)

## Goal

- Make the matrix subsystem genuinely faster — contiguous storage, flat-array APSP results.
- Isolate matrix creation and computation into coherent, testable types.
- Cancellation-aware algebra kernels.

## Status

🚧 In progress. A1, A2.0, A2 (APSP storage migration), A3 (contiguous storage), and A5
(cancellation-aware algebra kernels — plumbing shipped; making the flag delivery actually work is
tracked in WS15, not here) done. A7 scoped, moved to WS6, and done there (WS6.7). A4's
inventory corrected, ready to implement. A6 ready to implement. Neither started yet.

## Current Reality

- Matrix-related logic is scattered: code that constructs/populates `Graph`'s 11 named `Matrix`
  fields (`SIGMA, DM, sumM, invAM, AM, invM, WM, XM, XSM, XRM, CLQM`) lives across six different
  `src/graph/` slice directories, not one (`centrality/`, `distances/`, `cohesion/`, `matrices/`,
  `reachability/`, `reporting/`). Only one of the six is the `matrices/` directory that nominally
  owns this concern — this is A4's target.
- `Matrix`'s internal storage (flat buffer + row-pointer index, since A3) and its `QHash`-vs-`Matrix`
  APSP storage decision (since A2) are documented in `README_DEVELOPER_NOTES.md`'s "Matrix Storage"
  section — that's the current, durable reference; this doc tracks remaining/future work only.
- Matrix algebra methods (inverse, power iteration, etc.) run synchronously on the main thread with
  no cancellation support (see Known Issues below).

## Known Issues

### Cancellation (found during #52 Cancel-button fix)

**I1/I2 — ✅ Resolved, via [`roadmap_ws15_cancellation_progress_unification.md`](roadmap_ws15_cancellation_progress_unification.md) P1.**
A5 (below) shipped the `cancelCheck` plumbing these called for; live testing found the flag delivery
mechanism itself was broken (a queued cross-thread Qt connection that can't be processed while the
very loop it's meant to interrupt is blocking that thread's event loop) — a Graph/MainWindow wiring
problem, not a `Matrix` one, fixed in WS15 (atomic flag + `Qt::DirectConnection`, plus extending
`cancelCheck` into `ludcmp()` itself once testing showed that was the actual bottleneck). Live-verified.

**I3 — `writeMatrix()` had missing `file.close()` on cancel paths.** Fixed during #52.

**I4 — No cancellation support in `Matrix::distancesMatrix()`**, called from `writeMatrix()`
(`graph/reporting/graph_reports.cpp:5698`) for the `MATRIX_DISTANCES_EUCLIDEAN`/`HAMMING`/`JACCARD`/
`MANHATTAN`/`CHEBYSHEV` cases — no cancel guards yet.

### Correctness (found while building WS6.7's `kernel_matrix_v8`)

**C1 — `createMatrixAdjacencyInverse()` reports `invertible=true` for a mathematically singular
matrix** ([#269](https://github.com/socnetv/app/issues/269)). Its singularity check
(`graph/matrices/graph_matrix_adjacency.cpp:143`) is backwards: it runs the LU-decomposition
inversion regardless of whether the input can actually be inverted, then scans the *result* for any
nonzero off-diagonal entry to decide "not singular." A singular input still produces output — just
numerical garbage from a near-zero pivot — and that garbage usually has nonzero off-diagonal
entries too, so the check gets fooled. Confirmed directly: `TinyPath_N3_E2` (a 3-node path graph)
has two structurally-equivalent end nodes, so its adjacency matrix has two identical columns and is
therefore singular by construction (det=0); the computed "inverse" contained `1e+20`/`-1e+20`
entries (the classic near-zero-pivot blowup signature) while `invertible` still read `true`.
Anything trusting that flag before using the inverse — Information Centrality is the real
downstream consumer — could silently compute on garbage for singular/disconnected/tree-shaped
networks with no signal anything went wrong.

**Proposed fix, not yet implemented:** check pivot magnitude *during* LU decomposition
(`ludcmp()`, feeding `Matrix::inverse()` at `src/matrix.cpp:1256`) rather than inspecting the
inverted result afterward — flag singular the moment a pivot is smaller than some epsilon,
instead of running the division anyway. The epsilon should be a *relative* tolerance (scaled to
the matrix's own entry magnitudes), not a fixed absolute cutoff like `1e-11` — the right "basically
zero" threshold for a 0/1 adjacency matrix isn't necessarily right for a weighted matrix with
entries in the thousands. Not scoped into a specific milestone yet; natural fit alongside A5
(cancellation-aware algebra kernels) since both touch `Matrix::inverse()`'s internals.

## Milestones

### A1 — Inventory (done as part of this design pass)

The six-directory scatter and the 11-field catalog in Current Reality above is A1's output.
Prerequisite for A4 (isolating construction into `matrices/`).

### A2 — APSP Storage Migration ✅ Done (v3.7)

Handed off from [`roadmap_ws3_architecture_performance.md`](roadmap_ws3_architecture_performance.md)'s
"M1 continuation" section: `GraphVertex::m_distance`/`m_shortestPaths` (per-vertex `QHash`) caused
~73% of `DistanceEngine::finalize()`'s samples to go to hash lookups (profiled at 2,000N/40,000E).
Replaced with `Graph::m_apspDist`/`m_apspSigma` (`QHash<relation_id, Matrix>`, flat O(1) reads) — see
"Distance Engine" in `README_DEVELOPER_NOTES.md` for the current design.

### A2.0 — Empirical validation ✅ Done (v3.7)

`src/tools/matrix_storage_bench.cpp` (`BUILD_MATRIX_BENCH`) compared `QHash<int, QPair<int,qreal>>`
(mirroring `GraphVertex::m_distance`) against `Matrix`, at N=1,000/7,343 across connected/
disconnected/giant-component topologies. **Decision: GO** — `Matrix` wins lookup speed 22×-32× in
every configuration, and wins memory too for any topology with one dominant connected component
(what real networks look like); `QHash` only wins memory for artificial many-equal-islands graphs.
See "Matrix Storage" in `README_DEVELOPER_NOTES.md` for the durable finding and reasoning.

### A2 implementation ✅ Done (v3.7)

`Graph::m_apspDist`/`m_apspSigma` (`QHash<relation_id, Matrix>`) replaced the old per-vertex
`GraphVertex::m_distance`/`m_shortestPaths` QHash storage, migrated in three golden-verified steps
(dual-write → read-site migration → old storage removal). Real before/after benchmarks: no
measurable change at N≤500 (too small for the targeted O(N²) cost to matter), 15-17% faster at
N=1,000, 9% faster at N=7,343 (`geom.net`), 1-3% faster end-to-end in the GUI. See "Distance Engine"
in `README_DEVELOPER_NOTES.md` for where this storage lives today.

### A3 — `Matrix` contiguous storage (P1) ✅ Done

Replaced `MatrixRow*` (N separately-allocated row objects, one `new` per row) with one contiguous
`qreal*` buffer plus a precomputed row-pointer index (`m_rowPtr`) — see "Matrix Storage" in
`README_DEVELOPER_NOTES.md` for the design and why the row-pointer index (not just flattening) was
necessary to actually win. Zero external callers of `MatrixRow`/`operator[]`/`operator()` existed
outside `matrix.cpp` itself, so this was a pure internal refactor.

**Delivered:**
- Allocation count 7,344→1 for a `geom.net`-scale matrix (structural, not measured).
- Full doxygen pass across every `Matrix` method, with Big-O complexity notes on the
  non-trivial ones.
- Three real bugs fixed, found during that pass: `operator*` allocated the wrong result shape
  (OOB-write risk, zero current callers); `swapRows()`/`multiplyRow()` used `rows()` instead of
  `cols()` as row length (only correct for square matrices). `productSym()` marked `OBSOLETE`
  (dead code, no caller). `powerIteration()` re-verified — no bug found.
- Verified via `run_golden_compares.sh` (incl. WS6.7's `kernel_matrix_v8`, including a deliberate
  indexing-bug injection/reversion check) and `run_benchmarks.sh`, clean throughout.
- **End-to-end result: no measurable change**, and that's expected, not a shortfall — see "A win in
  isolated cell-access throughput does not guarantee an end-to-end win" in `README_DEVELOPER_NOTES.md`'s
  Matrix Storage section. `DistanceEngine`'s BFS traversal dominates total work over matrix writes,
  so a real isolated access-throughput win doesn't move the end-to-end number outside normal
  run-to-run noise on this workload.

**Construction/access-throughput evidence** — `src/tools/matrix_storage_bench.cpp`'s existing
`--structure matrix` branch already isolates exactly this (no new tool needed, reused as-is): built
against a pre-A3 worktree (`MatrixRow`) and current code (flat buffer + `m_rowPtr`), median of 5 runs
at N=7,343 across all three A2.0 topologies, checksums identical confirming no correctness change:

| Topology | Construct: old→new | Change | Lookup: old→new | Change |
|---|---|---|---|---|
| connected | 349ms→213ms | **39% faster** | 120ms→99ms | **18% faster** |
| disconnected | 120ms→43ms | **64% faster** | 121ms→98ms | **19% faster** |
| giant | 285ms→176ms | **38% faster** | 116ms→100ms | **14% faster** |

Consistent, real wins on both axes, in every topology — much lower run-to-run noise than the
end-to-end `DistanceEngine` check above, confirming those isolated wins are genuine even though they
don't surface end-to-end on that particular call site.

**`prominence --bench` support ✅ Done.** `run_benchmarks.sh` had no `prominence`-kernel case —
`AM`/`invAM`/`WM` (adjacency, inverse, walks) are only exercised via the **prominence** kernel
(Information Centrality, Eigenvector Centrality), which had no `--bench` support
(`socnetv_cli.cpp:133` hard-blocked `--bench` for anything but `--kernel distance`). Added: a
median-of-N loop in `kernel_prominence_v4.cpp` (mirroring `kernel_distance_v1.cpp`'s), the CLI
dispatch check relaxed to an allowlist, and a new `run_benchmarks.sh --type prominence` case
(`PROM_BA500_M3`, `Benchmark_BA_Directed_N500_m3.paj`, N=500 — not `geom.net`-scale, since
Information Centrality/Eigenvector Centrality are O(N³)-ish).

**Baseline recording deferred to the v3.7 tag, not done now.** Confirmed (checked out the `v3.6`
tag, rebuilt, reran the full suite) that every committed `perf_expected.env` was recorded during
the v3.6 cycle, matching the project rule that baselines are only re-recorded against a clean
tagged release, never `develop` HEAD — recording now would embed unrelated `develop`-only changes
into what's supposed to be a release snapshot. `PROM_BA500_M3` has no `v3.6` baseline to compare
against anyway (that build predates `--bench` support for this kernel entirely). Interim reading,
this machine, current `develop` HEAD, for reference only until the real recording happens at the
v3.7 tag: **`PROM_BA500_M3` median 40ms** (N=500, `--bench 20`).

Also found while checking this: `README__RELEASE_PROCEDURE.md` has no step at all for
(re-)recording perf baselines at release time — worth adding once this is done.

### A4 — Isolate construction into `matrices/`

**Inventory, checked directly — corrects A1's vague "five sites":** five matrix fields
are constructed outside `matrices/graph_matrix_adjacency.cpp`, across three files:
- `graph/distances/graph_distance_cache.cpp:53,168` — `SIGMA`, `DM`
- `graph/centrality/graph_centrality.cpp:102-103` — `WM`, `invM`
- `graph/reachability/graph_reachability_walks.cpp:56` — `XRM`

Two more fields are also constructed outside `matrices/`, via assignment/arithmetic operators
rather than `.resize()` — missed by the original count entirely:
- `XM`/`XSM` — `graph/reachability/graph_reachability_walks.cpp` (`XM = AM.pow(...)`, `XM *= AM`,
  `XSM += XM`)
- `CLQM` — `graph/cohesion/graph_cliques.cpp:139` (via `.zeroMatrix()`)

**Also found: `sumM` (one of the 11 named fields) is dead.** It's declared and cleared on graph
reset (`graph.cpp:327-330`) but never populated anywhere in the codebase — same category as the
`reserveShortestPaths()` dead-method finding from A2. Worth deciding whether to delete it outright
as part of A4, rather than migrating a field nothing ever writes to.

Migrate the seven real construction sites into the `matrices/` slice directory, one call site at a
time, golden-regression-verified after each move.

### A5 — Cancellation-aware algebra kernels (I1, I2) ✅ Done

`Matrix::inverse()` and `Matrix::powerIteration()` each gained an optional
`std::function<bool()> cancelCheck = nullptr` parameter, matching the existing
`Graph::progressCanceled()` convention already checked at ~30 call sites across `centrality/`,
`clustering/`, `distances/`, `generators/`, `cohesion/`. Checked once per outer-loop iteration
(`inverse()`'s per-column loop, `powerIteration()`'s `do`/`while` body) — a cancel breaks out
early rather than running to completion. Default `nullptr` keeps every other call site unaffected.

Wired at the three real call sites, each passing `[this]{ return progressCanceled(); }`:
`createMatrixAdjacencyInverse()` (`graph/matrices/graph_matrix_adjacency.cpp`, `"lu"` branch only —
also added the missing post-call `progressCanceled()` check per I2, so a canceled/partial inverse
isn't then treated as valid), `centralityInformation()`, `centralityEigenvector()`
(`graph/centrality/graph_centrality.cpp` — both already had a post-call check, just needed the
`cancelCheck` argument wired in).

**`Matrix::inverseByGaussJordanElimination()` deliberately untouched** — confirmed again it still
has zero reachable callers (same finding as before), so adding cancellation support to it would be
work spent on dead code.

**Verified:** `run_golden_compares.sh` and `run_benchmarks.sh` both pass unchanged (default
`nullptr`, so this is behavior-preserving when nothing is actually canceled).

**A5's own scope ends here — this plumbing is correct and complete.** Live GUI testing initially
found the `cancelCheck` lambdas inert in practice (a Graph/MainWindow signal-delivery problem, not a
`Matrix`/algebra one), fixed and live-verified in
[`roadmap_ws15_cancellation_progress_unification.md`](roadmap_ws15_cancellation_progress_unification.md)
(P1 — atomic flag + `Qt::DirectConnection`, plus extending cancellation into `ludcmp()` itself, the
actual O(n³) bottleneck `inverse()` wraps). WS15 also tracks a related crash bug (P3) found during
the same investigation and the progress-dialog-duplication finding (P2, moved there from WS7).

### A6 — Cancel guards in `writeMatrix()` (I4)

Add `progressCanceled()` checks to the `MATRIX_DISTANCES_EUCLIDEAN`/`HAMMING`/`JACCARD`/`MANHATTAN`/
`CHEBYSHEV` cases in `writeMatrix()` (`graph/reporting/graph_reports.cpp:5698`), matching the
pattern I3 already established for the file-handle-cleanup paths in the same function.

### A7 — Golden coverage for matrix operations ✅ Done, via WS6

Owned by [`roadmap_ws6_testing_ci_regression.md`](roadmap_ws6_testing_ci_regression.md)'s **WS6.7**
— a testing-harness concern, not a matrix-design one. `kernel_matrix_v8` shipped there: full
coverage audit, design decisions, and the empirical XSM/CLQM timing findings all live in that doc.

## Work Rules

- Performance claims must be measured (allocation counts, RSS, benchmark deltas), not asserted —
  A2.0 exists specifically to enforce this for the APSP migration.
- `run_golden_compares.sh` and `run_benchmarks.sh` must pass after every milestone.
- A3 (storage) is unblocked now that WS6.7's golden coverage has landed. A5 (cancellation) is
  independent and can proceed in parallel. A4 (isolation) should wait until A3 and A5 have landed
  so it moves settled code.
