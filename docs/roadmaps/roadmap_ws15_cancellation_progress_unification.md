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

🚧 In progress. P1 and P2 ✅ done. P3 (retire the linear dialog, close the dispatch gap): Groups
A/B/C ✅ done (all ~38 methods wrapped/deduplicated); Findings 1, 2, 3, 5, 7 resolved; Findings 4,
6, 8 open (see below). P4 (parallelization audit) ✅ done; implementation not started.

## Context

Split off from WS5 (A5's cancellation plumbing needed a working delivery mechanism) and WS7 (a
progress-dialog-duplication finding), then organized around the 4-property contract once it became
clear "dispatch," "cancellation," and "compute parallelism" were being treated as one bundled fix
when they're orthogonal.

## P1 — Atomic flag + `Qt::DirectConnection` ✅ Done

`Graph::m_progressCanceled` is now `std::atomic<bool>`; the Cancel button's `canceled()` signal
uses `Qt::DirectConnection` instead of the default queued cross-thread connection, so it lands
synchronously instead of waiting for a busy `graphThread` event loop that never frees up.
`Matrix::ludcmp()` (the O(n³) core of `inverse()`) got its own `cancelCheck`, since `inverse()`'s
own check is only reached after `ludcmp()` returns.

Known residual gaps: `Matrix::solve()`'s own `ludcmp()` call isn't wired, and `DistanceEngine`'s
parallel BFS deliberately skips cancellation in worker threads (performance-motivated).

## P2 — Global "graph busy" guard ✅ Done

Fixed a live, reproducible use-after-free crash: a still-running `graphThread` computation racing
`MainWindow::slotNetworkNew()`'s direct, undispatched `Graph::clear()`. `MainWindow::setAppBusy()`
disables `menuBar()`/`toolBar()`/`graphicsWidget` for the duration of every `runGraphOperationAsync`
call, and (since a later fix) every reachable `QAction` too — `menuBar()->setEnabled(false)` blocks
clicks but doesn't touch each action's own `isEnabled()`, so `Ctrl+N` could still fire a shortcut
mid-computation even with the widgets disabled. `setAppBusy()` now snapshots and restores only the
actions it itself disabled, so state legitimately disabled elsewhere (e.g. no network loaded) isn't
clobbered.

Known residual gap: the busy dialog still hides itself on Cancel-click before the computation
actually stops (Finding 8, below) — no longer a safety issue since the action-guard fix is
independent of dialog visibility, but a real UX-confusion one.

## P3 — Retire the linear progress-dialog system ✅ Done

Two progress-reporting systems coexisted in `MainWindow`: the legacy **linear** system
(`Graph::progressCreate()`/`progressUpdate()`/`progressFinish()` → a numeric `QProgressDialog`)
and the **indeterminate** busy dialog `runGraphOperationAsync()` already shows, backed by P2's
guard. A full call-site audit sorted every `Graph::` operation into:

- **Group A** (14 methods) — already fully wrapped; internal linear-dialog triads were pure
  redundancy, deleted.
- **Group B** (23 methods, 6 batches) — previously unwrapped, migrated onto
  `runGraphOperationAsync`; nested triads stripped once every call path into them was confirmed
  wrapped.
- **Group C** (18 call sites) — had neither system at all (mostly filters); wrapped from scratch.
- **Audit gap**: the original sweep mapped `progressCreate()` sites to their directly enclosing
  function only, missing triads inside shared helpers. Found and fixed:
  `writeMatrixHTMLTable()` (24 callers, had no `progressCanceled()` check at all — Cancel was a
  silent no-op), `layoutCircular()`, `writeCentralityCloseness()`.

`resetProgressCanceled()` is now also called centrally from `runGraphOperationAsync()` itself
(previously only `progressCreate()` and `DistanceEngine`'s sink called it) — otherwise deleting
Group A's own `progressCreate()` calls would have left some operations with no reset point at all.

**Findings from the audit** (✅ = resolved):

| # | Finding |
|---|---|
| 1 ✅ | 7 centrality/prestige primitives each reachable from 3 call paths with inconsistent wrapping — resolved once all 3 paths (own report / `vertexFindByIndexScore` / `layoutByProminenceIndex`) were confirmed wrapped. |
| 2 ✅ | `createMatrixAdjacency` fanned out to ~15 callers with mixed wrapping — resolved once Group B completed; own triad stripped. |
| 3 ✅ | `writeMatrixSimilarityMatching` double-fired the linear dialog. Fixed alongside Group A. |
| 4 | `Graph::vertexinfluenceRange()`/`vertexinfluenceDomain()` — zero callers anywhere in `src/`, dead code. Not deleted; needs user confirmation. |
| 5 ✅ | `writeMatrixWalks` had two entry points with different wrapping — resolved once `slotAnalyzeWalksLength` was migrated. |
| 6 | `Graph::writeReachabilityMatrixPlainText()` — zero live callers (only a stale doc-comment). Not deleted; needs user confirmation. |
| 7 ✅ | Finding 1's 7 primitives also double-fired the linear dialog once wrapped (same shape as Finding 3) — visible as a stuck/blank dialog on macOS, not just redundancy. Fixed alongside Finding 1. |
| 8 | Busy dialog hides itself on Cancel-click before the computation stops (Qt's `QProgressDialog` built-in Cancel button appears to bypass `setAutoClose`/`setAutoReset`). UX-confusion only, not a safety issue. Not fixed. |

## P4 — Parallelization audit ✅ Audit done, implementation not started

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

- **Findings 4 and 6**: decide with the user whether to delete the two confirmed-dead methods.
- **Finding 8**: decouple the busy dialog's Cancel button from Qt's built-in hide-on-cancel
  behavior.
- **P4 implementation**: decide which parallelization candidates to act on, if any.

## Work Rules

- P1 is pure infrastructure (behavior-preserving when nothing is canceled) — golden/benchmarks
  must show zero change.
- P2 got explicit design sign-off before implementation, per the project's plan-before-code rule.
- Group A deletions and Group B migrations were each independently golden/benchmark-verified and
  live-verified, one call site (or small batch) at a time.
- P4's audit was research first, implementation second.
- No GitHub issue for any of this (unreleased 3.7-cycle behavior) — fix directly.
- Once P4 lands (or is explicitly parked): add a changelog entry, update WS5's A5 section and
  WS7's status line to point here instead of duplicating content.
