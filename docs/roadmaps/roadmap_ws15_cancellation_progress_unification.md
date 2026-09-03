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

🚧 In progress. P1-P3 ✅ done — linear progress-dialog system retired, exactly one progress dialog
now exists app-wide. P4's audit done; `centralityDegree()`, `isSymmetric()`,
`clusteringCoefficient()`, and `graphTriadCensus()` parallelized so far, rest of the candidates
not started. See What WS15 Delivered below.

## What WS15 Delivered

### P1 — Atomic flag + `Qt::DirectConnection` ✅ Done

`Graph::m_progressCanceled` is now `std::atomic<bool>`; the Cancel button's `canceled()` signal
uses `Qt::DirectConnection` instead of the default queued cross-thread connection, so it lands
synchronously instead of waiting for a busy `graphThread` event loop that never frees up.
`Matrix::ludcmp()` (the O(n³) core of `inverse()`) got its own `cancelCheck`, since `inverse()`'s
own check is only reached after `ludcmp()` returns.

Known residual gaps: `Matrix::solve()`'s own `ludcmp()` call isn't wired, and `DistanceEngine`'s
parallel BFS deliberately skips cancellation in worker threads (performance-motivated).

### P2 — Global "graph busy" guard ✅ Done

`MainWindow::setAppBusy()` disables `menuBar()`/`toolBar()`/`graphicsWidget`/`leftPanel` (the
toolbox panel) and every reachable `QAction`, for the duration of every `runGraphOperationAsync`
call — snapshotting and restoring only what it itself disabled, so state legitimately disabled
elsewhere isn't clobbered. Covers container-level `setEnabled(false)`, individual `QAction`s
(whose own `isEnabled()`, e.g. keyboard shortcuts, a container disable alone doesn't reach), and
the toolbox's `QComboBox`es specifically (not `QAction`s, so outside that sweep otherwise — see
`roadmap_ws5_matrices_modernization.md`'s A6 section).

### P3 — Retire the linear progress-dialog system ✅ Done

Every `Graph::` operation reachable from the GUI now dispatches through `runGraphOperationAsync()`'s
single indeterminate busy dialog. The legacy linear system (`Graph::progressCreate()`/
`progressUpdate()`/`progressFinish()` → a separate numeric `QProgressDialog`) is fully retired,
including `DistanceEngine`'s own nested dialog — `DistanceEngine::compute()` already self-closes
its dialog internally, so removing dialog creation there left `resetCancellation()` (renamed from
`progressCreate()`) as its only remaining job. `randomNetErdosCreate()`'s `--interactive-script
erdos`/`erdos-m` benchmark path, the one caller that bypasses `runGraphOperationAsync`, calls
`resetProgressCanceled()` directly instead, since it has no other reset point. Exactly one progress
dialog exists in the app now, always — and it stays visible through Cancel: Qt's
`QProgressDialog::cancel()`, wired to the built-in Cancel button, unconditionally hides the dialog
before emitting `canceled()` (`setAutoClose`/`setAutoReset` don't gate that path), so a second
`canceled()` connection re-shows it, relabels it "Canceling...", and disables it until the
operation's own completion continuation tears it down for real.

### P4 — Parallelization audit ✅ Audit done, four implementations landed, rest open

Audited every long-running operation in `src/graph/`'s algorithm slices against all four contract
properties, judging property 4 by real algorithm structure (independent per-source/per-node work
vs. an inherently sequential dependency chain), not a grep pass.

**Best parallelization candidates** (clear win, APSP-shaped, per-vertex/per-row independent,
read-mostly): `centralityDegree`, `graphTriadCensus`, `clusteringCoefficient`, the O(N²)
matrix-fill loops following `graphDistancesGeodesic()` (`graphMatrixShortestPathsCreate`,
`graphMatrixDistanceGeodesicCreate`, `createMatrixReachability`, `createMatrixAdjacency`),
`centralityClosenessIR`/`prestigeDegree`/`prestigeProximity`. `createMatrixSimilarityMatching`/
`Matrix::distancesMatrix()`/`pearsonCorrelationCoefficients()` are good fits too but need a
`cancelCheck`-style `Matrix` API change first.

**Poor candidates** (inherently sequential): `Matrix::inverse()`/LU decomposition,
`graphClusteringHierarchical` (agglomerative merging), the outer particle-selection loop in
`layoutForceDirectedKamadaKawai`, preferential-attachment growth in `randomNetScaleFreeCreate`.
The random generators generally split into a parallelizable "decide" phase and a serial "apply"
phase (`edgeCreate()` mutates shared state) — not a drop-in `blockingMap`.

**Worst remaining cancellation gaps** (long-running, coarse-only or zero mid-loop checks):
`prestigePageRank`'s convergence loop, `graphTriadCensus`'s O(N³) inner loops,
`createMatrixSimilarityMatching` (one opaque uncancellable step), `randomNetRegularCreate`'s
unbounded edge-randomization retry loop, `graphCliques`' Bron-Kerbosch recursion (checked only at
recursion depth 1, deliberately, to avoid flooding the event loop — an accepted trade-off).
`graphConnectivity()` (#278, WS11's #7) is the worst of these: zero `progressCanceled()` checks at
all, on an O(N²) max-flow sweep — confirmed hanging 30+ minutes uncancellable on a real N=2000
sparse network. Missed by this audit at the time it was written; tracked in WS11 now.

Already parallel: `graphDistancesGeodesic` (→ `DistanceEngine`, `QtConcurrent::blockingMap`);
Betweenness/Brandes centrality rides along in the same pass.

Not yet decided whether/when to act on any of this — it's a map, not a commitment. Any
parallelization work still needs its own golden/benchmark evidence (same discipline as WS5
A2.0/A3) before being called a real improvement.

### P4 — First implementation: `centralityDegree()` parallelized

`Graph::centralityDegree()` now maps its outer per-vertex loop via `QtConcurrent::blockingMap`,
same shape as `DistanceEngine`'s APSP. Found and fixed a real, pre-existing thread-safety bug
along the way: `Graph::edgeExists()` (called from inside the parallel loop) wrote its result
through two `Graph`-instance member fields (`edgeWeightTemp`/`edgeReverseWeightTemp`) instead of
locals - harmless single-threaded, but a data race across worker threads. Converted to locals;
nothing outside `edgeExists()` read those fields, so this was a pure win with no external effect,
and it de-risks every future P4 candidate that also reads edges concurrently.

Golden/benchmark evidence: `./scripts/run_golden_compares.sh` and `run_benchmarks.sh` both clean.
No measured wall-clock win at tested scale (N≈2000 on `2000actors-40000edges.graphml`: DC alone
measured 0ms both before and after - too cheap an O(N²) hash-lookup loop to register at this
size). Landed anyway: the value here is validating the pattern and fixing `edgeExists()`, ahead of
parallelizing costlier candidates (`graphTriadCensus`, the matrix-fill loops) where the win should
actually be measurable.

### P4 — Second implementation: `isSymmetric()` parallelized

Found while fixing `clusteringCoefficientLocal()`'s own internal `this->isSymmetric()` call (not
in the original audit's named candidate list, but the same per-vertex/read-mostly shape). Same
`m_graphIsSymmetric` shared-write hazard as `centralityDegree()`'s inline symmetry check, fixed the
same way (`QAtomicInteger` OR-reduce). Can't short-circuit on first asymmetry under
`blockingMap` (no shared control flow across worker threads), so every vertex is now always
checked - strictly more work in the asymmetric case, never wrong, no slower in the common
(symmetric) case where every vertex had to be checked anyway. This also removes the pre-existing
duplication where `centralityDegree()` had to reimplement this same check inline instead of
calling `isSymmetric()`, now that it's safe to call from a parallel context too.

Also fixed in the same pass: `GraphVertex::reciprocalEdgesHash()` had the same
member-field-as-scratch-space hazard as `edgeExists()` (`m_reciprocalEdges`, never read
externally) - converted to a local. Found and removed one genuinely dead field
(`m_reciprocalLinked`, unused anywhere) via a `-Wunused-private-field` warning surfaced while
touching neighboring fields.

### P4 — Third implementation: `clusteringCoefficient()` parallelized

`Graph::clusteringCoefficient()`'s outer per-vertex loop now maps via `blockingMap`, depending on
the now-safe `isSymmetric()` above: `clusteringCoefficientLocal()` used to call
`this->isSymmetric()` internally on every invocation, which would race across worker threads on
first (cache-cold) use, so it now takes `isSymmetric` as a parameter instead - the caller computes
it once, sequentially, before the parallel step. Class/min/max/average/variance bookkeeping stays
sequential afterward, reading back each vertex's now-cached `CLC()`.

Measured (not assumed), same dataset as above: 499ms sequential vs. 84ms parallel, **~5.9x** -
isolated via a temporary edit to `kernel_clustering_v6.cpp`'s timer (reverted before committing).
Unlike `centralityDegree()`, this candidate does show a real, measurable win: each vertex's
`clusteringCoefficientLocal()` call is O(k²) in its neighbourhood size, not a flat O(N) sum, so
there's genuinely more per-vertex work for the parallel step to amortize.

### P4 — Fourth implementation: `graphTriadCensus()` parallelized

`Graph::graphTriadCensus()`'s outer vertex loop now maps via `blockingMap` over vertex positions
(needed for the existing `v2 = v1+1`/`v3 = v2+1` positional pairing, unlike the other three
candidates' simpler per-vertex-pointer mapping); each worker thread runs its own `v2`/`v3` loops
exactly as before, since every `(v1,v2,v3)` triad classification only reads edges via
`GraphVertex::hasEdgeTo()` and has no dependency on any other triad. The one shared state -
`triadTypeFreqs[16]`, previously incremented directly (non-atomic `++`) by
`triadType_examine_MAN_label()` - would race once reached from multiple worker threads
concurrently. Fixed by having that function increment one of 16 `QAtomicInteger<int>` counters
instead (`fetchAndAddOrdered`), with a sequential pass copying the final counts into
`triadTypeFreqs` after `blockingMap` returns. The one existing mid-loop `progressCanceled()` check
(previously once per outer vertex, already coarse against O(N³) work) moves to a single check
before the parallel step starts, since cancel can't be delivered while `graphThread`'s event loop
is blocked inside `blockingMap` anyway - same accepted tradeoff as the other three candidates.

Measured (not assumed) on a 1000-node/10,000-edge network (`1000actors-10000arcs.graphml`),
isolated via the same temporary-kernel-timer method as above: **68.2s sequential vs. 15.4s
parallel, ~4.4x** - the largest measured win of WS15 P4 so far, consistent with this being the one
O(N³) candidate among the four done to date. Triad classification output (all 16 class counts,
166,167,000 total triads) verified identical between the sequential and parallel runs.

## What Remains Open

- **P4 implementation, remaining candidates**: the O(N²) matrix-fill loops after
  `graphDistancesGeodesic()`, `centralityClosenessIR`/`prestigeDegree`/`prestigeProximity` - not
  yet started. Decide which to act on next.

While investigating P3's Cancel-button fix, tracing a distance-based analysis end to end also
surfaced a reproducible crash in the `--interactive-script` command dispatcher (a script-ordering
race, independent of anything above) — found, fixed, and documented in
`roadmap_ws12_cli_scripting_mode.md`, not here.

## Work Rules

- No GitHub issue for any of this (unreleased 3.7-cycle behavior) — fix directly.
- Once P4 lands (or is explicitly parked): add a changelog entry, update WS5's A5 section and
  WS7's status line to point here instead of duplicating content.
