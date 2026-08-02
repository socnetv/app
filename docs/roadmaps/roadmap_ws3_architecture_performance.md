# Architecture & Performance Roadmap (WS3)

## Goal

Fix specific, measured performance and correctness problems in `Graph`/`GraphVertex` — not build a
separate domain-model layer for its own sake. (Originally scoped around introducing a domain model;
that turned out unnecessary — every real win here came from fixing a specific, measured problem
directly, not from restructuring on the assumption that separating concerns is inherently worth it.)

## Status

✅ Done — see [`ARCHITECTURAL_REFACTORING_ROADMAP.md`](../ARCHITECTURAL_REFACTORING_ROADMAP.md).
Current `DistanceEngine`/threading architecture lives in
[`README_DEVELOPER_NOTES.md`](../README_DEVELOPER_NOTES.md), not here.

## What WS3 Delivered

- **M1 — `DistanceEngine` parallelization** ✅ — source loop parallelized via
  `QtConcurrent::blockingMap`; measured **2.7×–8.3×** speedup (Debug build, 24-core Linux), all
  golden baselines passing. Architecture (`PerSourceScratch`/`ThreadLocalState`, call flow) now
  documented in README's "Distance Engine" section.
- **M1 continuation (APSP storage)** ✅ — delegated to and completed by
  [WS5's A2](roadmap_ws5_matrices_modernization.md): relation-keyed flat matrices
  (`Graph::m_apspDist`/`m_apspSigma`) replacing per-vertex `QHash` storage on `GraphVertex`.
- **#254/#263 — GUI freeze during long computations** ✅ — root cause: `MainWindow` called `Graph`
  methods directly/synchronously instead of dispatching onto `graphThread`. Fixed with
  `MainWindow::runGraphOperationAsync()` (now the standard pattern — see README), rolled out across
  31 entry points. Two real bugs found along the way: a `QProgressDialog::close()` call that
  silently marked successful completions as cancelled (fixed via `reset()` instead), and a
  UI-façade chart-building crash once code genuinely started running on `graphThread` (fixed with
  `Graph::runOnGuiThread()`, see WS2).
- **#266 — Walks Total numeric display fix** ✅ — walk counts at high matrix powers exceed
  `double`'s precision; `writeMatrixHTMLTable()` now uses scientific notation above magnitude 1,000
  instead of printing false-precision digits.
- **#249/#253 — Viewport auto-fit and resize debouncing** ✅ — `resizeEvent` debounced to one
  `canvasSizeSet` per stable stop; zoom/reset decoupled from the slider chain; `zoomToFit()` snaps
  to 100% when within a small margin of it. Canvas/`GraphicsWidget` work now tracked under
  [WS10](roadmap_ws10_graphicswidget_overhaul.md).
- **M2 — `GraphVertex` `QObject` removal + edge-visibility signal batching** ✅ — dropped
  `GraphVertex`'s `QObject` inheritance (its one signal now relayed via plain method calls to
  `Graph`); measured ~16 bytes/vertex saved (~117 KB on `geom.net`'s 7,343 nodes) plus eliminating
  `QObjectPrivate`'s separate per-vertex heap allocation. Batched the two bulk edge-visibility call
  sites (`relationSet()`, `edgeFilterUnilateral()`) from one signal per edge to one per batch.
- **GraphicsWidget Performance and Code Quality Overhaul (#250)** ✅ — tracked in full under
  [WS10](roadmap_ws10_graphicswidget_overhaul.md).

## Investigated and Deferred

- **M3 — move pure data containers out of UI/Qt dependencies.** Two candidate fields were checked
  and found already correctly placed, not misplaced as originally assumed: `canvasWidth`/
  `canvasHeight` are genuine headless layout parameters (read by layout generators and the headless
  CLI path, not UI leftovers), and `m_clickedEdge`/`m_vertexClicked` already live in the correct
  layer (`src/graph/ui/`'s click-resolution computation, not passive UI bookkeeping). The one real
  candidate — giving a structural adapter real ownership of `VList m_graph`/`vpos` instead of just
  reading through `Graph` — was checked and rejected: `vpos` is touched at ~140 call sites across 18
  files, ownership transfer would buy zero performance (same lookup, same complexity, wherever the
  hash lives), and there's no currently-blocked test behind it. Deferred indefinitely; revisit only
  if a concrete need appears.
- **M4 — relocate `Graph`'s cache flags into explicit cache objects.** The stated motivation (~20
  `calculated*` flags with "no shared invalidation mechanism") was checked directly against
  `Graph::setModStatus()` and found false: a shared mechanism already exists, called from 66 sites,
  and correctly invalidates at every real structural mutator checked. No live staleness bug found.
  What's left is coarse-grained invalidation (safe, but recomputes more than strictly necessary) and
  a real-but-not-currently-felt maintenance tax (new analyses hand-edit the same shared reset
  block) — not enough on its own to justify the refactor. Deferred indefinitely, same as M3.
- A `GraphModel` structural adapter was built as a supposed M2 prerequisite, shipped, found to be
  unused by the commit it was meant to enable, and removed the next day (`b9508c17`) — see git
  history for the full trace, not repeated here.

## What Remains Open

Nothing actively scoped. M3/M4 are parked (see above) — pick back up only if a concrete need
appears, not preemptively.
