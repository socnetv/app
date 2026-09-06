# App Responsiveness Contract (WS15)

Dispatch, Cancellation, Busy-Guard & Parallelization

## Goal

Make "the app is responsive" a checkable property instead of a recurring, never-quite-finished
fix: define responsiveness as four independent, per-operation properties, and audit every
long-running `Graph::` operation against all four, rather than fixing whatever's currently visible
and calling it done.

**The contract — four properties, numbered to match the milestone (P1-P4) that owns each one:**

1. **Working cancellation (P1)** — a `cancelCheck` wired at genuinely fine-grained points, using
   the atomic-flag + `Qt::DirectConnection` mechanism.
2. **Busy-guard coverage (P2)** — menu/toolbar/canvas/shortcuts disabled for the duration, via
   `setAppBusy()`. Automatic once (3) is true.
3. **Non-blocking dispatch (P3)** — wrapped in `runGraphOperationAsync` (or equivalent).
4. **Internally parallelized where the algorithm structure allows it (P4)** — `QtConcurrent`,
   following `DistanceEngine`'s APSP precedent. Not every algorithm qualifies (e.g. LU
   decomposition is sequential); the audit records which do and how.

An operation can satisfy any subset of these independently — that's a legitimate state to report,
not something to paper over.

## Status

✅ Complete. All four properties delivered across the whole `src/graph/` algorithm surface.

## What WS15 Delivered

### P1 — Working cancellation

`Graph::m_progressCanceled` is `std::atomic<bool>`; the Cancel button's `canceled()` signal uses
`Qt::DirectConnection` so it lands synchronously instead of waiting on a busy `graphThread` event
loop. `Matrix::ludcmp()` (the O(n³) core of `inverse()`) got its own `cancelCheck`.

Known residual gaps: `Matrix::solve()`'s own `ludcmp()` call isn't wired, and `DistanceEngine`'s
parallel BFS deliberately skips cancellation in worker threads (performance-motivated, same
tradeoff P4 later adopted everywhere else too).

### P2 — Global "graph busy" guard

`MainWindow::setAppBusy()` disables `menuBar()`/`toolBar()`/`graphicsWidget`/`leftPanel` and every
reachable `QAction` for the duration of every `runGraphOperationAsync` call, snapshotting and
restoring only what it itself disabled. Covers container-level disables, individual `QAction`s
(keyboard shortcuts), and the toolbox's `QComboBox`es specifically (see
`roadmap_ws5_matrices_modernization.md`'s A6 section).

### P3 — Retired the linear progress-dialog system

Every `Graph::` operation reachable from the GUI dispatches through `runGraphOperationAsync()`'s
single indeterminate busy dialog. The legacy linear system
(`progressCreate()`/`progressUpdate()`/`progressFinish()` → a numeric `QProgressDialog`) is fully
retired, including `DistanceEngine`'s own nested dialog. Exactly one progress dialog exists in the
app now, and it stays visible through Cancel (relabels "Canceling...", disables itself until the
operation's completion continuation tears it down).

### P4 — Parallelization: audit + 11 functions converted

Audited every long-running operation in `src/graph/`'s algorithm slices against real algorithm
structure (independent per-source/per-node work vs. an inherently sequential dependency chain),
then parallelized every candidate the audit named, via `QtConcurrent::blockingMap`:

- `centralityDegree`, `isSymmetric`, `clusteringCoefficient`, `graphTriadCensus`
- The four matrix-fill operations following `graphDistancesGeodesic()`: `graphMatrixShortestPathsCreate`,
  `graphMatrixDistanceGeodesicCreate`, `createMatrixReachability`, `createMatrixAdjacency`
- `centralityClosenessIR`, `prestigeDegree`, `prestigeProximity`

**Recurring hazard classes found and fixed** (the real substance of this work — each was a
pre-existing bug, independent of parallelization, only surfaced by attempting it):

- **Member fields used as scratch/return storage** instead of locals — safe single-threaded, a
  data race once read/written from worker threads. Hit in `Graph::edgeExists()`
  (`edgeWeightTemp`/`edgeReverseWeightTemp`) and `GraphVertex::reciprocalEdgesHash()`
  (`m_reciprocalEdges`); both converted to locals. Also removed one genuinely dead field found
  along the way (`GraphVertex::m_reciprocalLinked`, unused anywhere).
- **Lazy compute-and-cache methods called concurrently on first use** — `isSymmetric()`'s
  internal cache and the inline `m_graphIsSymmetric` flag it duplicated in `centralityDegree()`/
  `prestigeDegree()` would race if every worker thread's first call landed at once. Fixed with
  `QAtomicInteger<bool>` OR-reduce (no shared control flow across `blockingMap` workers, so every
  vertex is always checked rather than short-circuiting on first asymmetry — strictly more work
  in the asymmetric case, never wrong). `clusteringCoefficientLocal()` instead takes `isSymmetric`
  as a parameter, computed once sequentially before the parallel step.
- **Shared accumulators mutated inline during the per-vertex loop** — `sum*`, `resolveClasses()`
  (mutates a shared `QHash`), and min/max tracking (plain compare-and-assign) all race under
  concurrent writers. Fixed by computing only the per-vertex value inside `blockingMap` and moving
  all bookkeeping to a sequential pass afterward, reading back each vertex's now-cached score —
  the pattern used by `clusteringCoefficient`, `centralityClosenessIR`, `prestigeDegree`,
  `prestigeProximity`.
- **Sequential row/column counters** in the four matrix-fill functions blocked mapping the outer
  loop over threads (not a `Matrix` data race — confirmed safe for concurrent writes to disjoint
  cells, flat raw-array storage, no locking/COW). Fixed with a new helper,
  `Graph::compactedMatrixIndex(dropIsolates)`, computed once sequentially: maps each vertex's
  position to its compacted row/column index, so each worker thread looks up its own index
  independently. `graphTriadCensus()`'s 16-bucket `triadTypeFreqs` accumulator got the same
  treatment via `QAtomicInteger<int>` counters instead (a genuine reduction, not an index lookup).

**Also fixed along the way, independent of parallelization**: `Graph::prestigeDegree()` leaked one
`QHash` per vertex (reused pointer overwritten each iteration without freeing the previous
allocation); `socnetv-cli`'s numeric CLI options (`-f`/`--format` and 13 others) silently
misparsed invalid input via `QString::toInt()`/`toDouble()` instead of failing.
`graphMatrixShortestPathsCreate()`'s SIGMA matrix had no golden/CLI coverage at all before this
work (no accessor existed) — added `Graph::matrixShortestPaths()` and a `"shortest_paths"`
category to `kernel_matrix_v8`'s golden coverage.

**Measured, not assumed** (1000-node/10,000-edge network unless noted; isolated via temporary
kernel-timer edits or `git stash`, reverted before committing; correctness verified via bit-identical
JSON diffs against sequential output):

| Function(s) | Sequential | Parallel | Speedup |
|---|---|---|---|
| `graphTriadCensus` (O(N³)) | 68.2s | 15.4s | ~4.4x |
| 4 matrix-fill operations (full kernel) | 28.2s | 14.6s | ~1.9x |
| `centralityClosenessIR` + `prestigeProximity` | 297ms | 64ms | ~4.6x |
| `clusteringCoefficient` (2000-node/40,000-edge) | 499ms | 84ms | ~5.9x |
| `centralityDegree`, `prestigeDegree` | — | — | No measurable win — O(N) hash-lookup, too cheap to register at this scale |

**Poor candidates, not parallelized** (inherently sequential): `Matrix::inverse()`/LU
decomposition, `graphClusteringHierarchical` (agglomerative merging), the outer particle-selection
loop in `layoutForceDirectedKamadaKawai`, preferential-attachment growth in
`randomNetScaleFreeCreate`. The random generators generally split into a parallelizable "decide"
phase and a serial "apply" phase (`edgeCreate()` mutates shared state) — not a drop-in
`blockingMap`.

**Cancellation gaps found by the same audit, tracked elsewhere**: `prestigePageRank`'s convergence
loop, `graphTriadCensus`'s O(N³) inner loops (now single-checked before the parallel step, same as
every P4 candidate), `createMatrixSimilarityMatching` (one opaque uncancellable step),
`randomNetRegularCreate`'s unbounded retry loop, `graphCliques`' Bron-Kerbosch recursion (checked
only at recursion depth 1 deliberately). `graphConnectivity()` was the worst — zero
`progressCanceled()` checks on an O(N²) max-flow sweep, confirmed hanging 30+ minutes uncancellable
on a real N=2000 network — filed as #278, tracked in WS11.

**Not parallelized, needs its own API work first**: `createMatrixSimilarityMatching`,
`Matrix::distancesMatrix()`, `pearsonCorrelationCoefficients()` are good `blockingMap` fits but
need a `cancelCheck`-style `Matrix` API change before that's worth doing.

A crash found while tracing a distance-based analysis end to end during P3 (an
`--interactive-script` command-dispatcher script-ordering race, unrelated to cancellation) is
documented in `roadmap_ws12_cli_scripting_mode.md`, not here.

## What Remains Open

Nothing from this workstream's own scope. Any future parallelization work
(`createMatrixSimilarityMatching` and friends, above) would be a new, separately-scoped effort,
not a continuation of this list.
