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
now exists app-wide. P4's audit done; implementation not started. See What WS15 Delivered below.

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

### P4 — Parallelization audit ✅ Audit done, implementation not started

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

Already parallel: `graphDistancesGeodesic` (→ `DistanceEngine`, `QtConcurrent::blockingMap`);
Betweenness/Brandes centrality rides along in the same pass.

Not yet decided whether/when to act on any of this — it's a map, not a commitment. Any
parallelization work still needs its own golden/benchmark evidence (same discipline as WS5
A2.0/A3) before being called a real improvement.

## What Remains Open

- **P4 implementation**: decide which parallelization candidates to act on, if any.

While investigating P3's Cancel-button fix, tracing a distance-based analysis end to end also
surfaced a reproducible crash in the `--interactive-script` command dispatcher (a script-ordering
race, independent of anything above) — found, fixed, and documented in
`roadmap_ws12_cli_scripting_mode.md`, not here.

## Work Rules

- No GitHub issue for any of this (unreleased 3.7-cycle behavior) — fix directly.
- Once P4 lands (or is explicitly parked): add a changelog entry, update WS5's A5 section and
  WS7's status line to point here instead of duplicating content.
