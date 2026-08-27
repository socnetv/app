# GraphicsWidget — Canvas Rendering & Features (WS10)

## Goal

Keep `GraphicsWidget` (`src/graphicswidget.h`/`.cpp`, the `QGraphicsView` subclass that owns the
canvas — node/edge scene items, selection, zoom/rotation, guides) fast at large network sizes and
capable of new canvas-drawing features, as an ongoing concern separate from `Graph`/`GraphVertex`
domain-model work.

## Status

Phase 1 (#250, a correctness/hot-path/structural cleanup pass), the #260 canvas-clear performance
fix, and the rendering-performance regression kernel (WS6.6) are shipped — see What WS10 Delivered
below. The rest of the Performance Checklist and the whole Feature Checklist remain open.

## Background

This was originally tracked as a subsection of the [Architecture & Performance Roadmap
(WS3)](roadmap_ws3_architecture_performance.md); split out into its own workstream (WS10) once #250
completed, since canvas/`GraphicsWidget` work is a structurally separate, ongoing concern from
WS3's domain-model work (`Graph`/`GraphVertex`) — not a one-off cleanup with a defined end.

**Scope widened beyond pure performance** to cover `GraphicsWidget` generally — new canvas-drawing
features (e.g. #22) belong here too, not just rendering-cost reduction, since they touch the same
class and the same `QGraphicsScene`/`QGraphicsView` machinery.

Correctness bugs, hot-path allocation/scan reductions, and structural changes to `GraphicsWidget`
were Phase 1's territory (done, see below). **What Phase 1 did not do:** none of it changed the
actual paint/geometry cost of rendering a large network — it removed waste (redundant allocations,
scans, transform applications) without touching the underlying per-item rendering model. If the
canvas still feels slow on large networks, that's expected — the real rendering-cost work is the
Performance Checklist below. New canvas-drawing capabilities (not performance-motivated) are
tracked separately in the Feature Checklist.

## What WS10 Delivered

### Phase 1 — GraphicsWidget Performance and Code Quality Overhaul (#250) ✅ Complete

Correctness fixes, hot-path allocation/scan reductions, and structural changes across
`graphicswidget.cpp`, closed out with a full Doxygen pass and a dead-code sweep. Issue
[#250](https://github.com/socnetv/app/issues/250) is closed. Worth remembering from this pass:

- **Integer edge keys.** `createEdgeName()` built a `"relation:v1>v2"` string with several heap
  allocations on every edge operation. Replaced with `GraphicsWidget::edgeKey()`, a
  decimal-digit-packed `quint64` (`relation * 10^16 + v1 * 10^8 + v2` — a 3-digit relation field and
  two 8-digit node-number fields), chosen over a bit-packed or mixing-hash key specifically for
  debuggability (digits read left to right in the same order as the old string) and because
  positional encoding makes collisions between distinct `(relation, v1, v2)` triples structurally
  impossible, not just statistically unlikely — backed by `Q_ASSERT_X` bounds checks in `edgeKey()`.
- **Rotate-button regression, found and fixed during this pass.** Guarding the rotation-slider
  signal chain with `QSignalBlocker` (to stop `zoomToFit()`/`reset()` double-applying the transform)
  broke the rotate buttons: unlike `zoomIn`/`zoomOut`, `rotateLeft`/`rotateRight` never called
  `changeMatrixRotation()` directly — the transform was only ever applied as a side effect of the
  now-blocked slider's `valueChanged`. Fixed by making `rotateLeft`/`rotateRight` call
  `changeMatrixRotation()` directly, same pattern as the zoom methods.
- **`edgesHash.reserve(500000)` at startup, removed rather than resized.** A "reserve based on real
  counts once known" fix was considered and dropped: `Graph::signalGraphLoaded` only fires *after*
  all `createNode`/`createEdge` calls have already populated the hashes, so it can't size the load
  that's already happening. Given `QHash`'s amortized-O(1) growth, a fixed reserve only saved a
  handful of cheap rehashes on very large loads while costing every small/medium network (the
  common case) a bucket array sized for 500,000 entries at construction — removed outright.
- **Dead code found via a systematic zero-caller sweep** (`grep -rn` per method across all of
  `src/`, not just the originally-flagged items): `hasNode()` and `setNodeSizeAll()` had no callers
  anywhere in the tree — both deleted outright rather than refactored.
- Also: single-probe hash lookups (`contains()`+`value()` → `value(key, nullptr)`) across 15+ sites,
  `qDebug()` → `qCDebug(lcGW)` in hot paths (`wheelEvent`, `mousePressEvent`, zoom/scale handlers),
  guide items tracked in a maintained `QList<GraphicsGuide*>` instead of full `scene()->items()`
  scans, `selectAll()` fixed to use scene coordinates instead of viewport pixels, and a full Doxygen
  `@brief` pass across all 78 methods plus all 13 signals (which have no `.cpp` body to document
  against).

### Post-Phase-1 fix — canvas-clear performance (#260) ✅ Complete

Clearing a large network (File → New, or loading a new file over one already displayed) was slow —
confirmed 30s to over a minute on a 2000-node/40,000-edge network. Two distinct causes, both fixed:

- **`scene()->clear()` destructor cascade (WS10 territory).** `GraphicsNode::~GraphicsNode()`
  manually `delete`d every one of its edges before self-removing, and each
  `GraphicsEdge::~GraphicsEdge()` then did its own individual `removeRefs()` (O(n) `std::list`
  scan to unlink from the *other* endpoint's edge list). Fixed by adding
  `GraphicsWidget::isClearing()`, set for the duration of `GraphicsWidget::clear()`; when set, both
  the node destructor's manual edge-deletion loop and `removeRefs()` are skipped entirely, since
  `QGraphicsScene::clear()` already tears down every remaining item itself. Real, but not the
  dominant cost.
- **MainWindow signal re-entrancy (not WS10 territory, noted here only for the full picture).**
  Live profiling (`sample` against the actual process, driven reproducibly via the
  `--interactive-script` CLI flag from #261) showed the actual dominant cost was a
  `QComboBox::setCurrentIndex()` call in `MainWindow::slotNetworkChanged()` missing a
  `blockSignals()` guard — it re-entered `slotEditEdgeMode()`, which forced a synchronous
  full-canvas repaint via `statusMessage()` while the old (still fully rendered) network was still
  on screen. Fixed in `mainwindow.cpp`, not `graphicswidget.cpp` — see #260 for the full write-up.

### Rendering-performance regression coverage (WS6.6) ✅ Done (2026-07-31)

The golden harness covered computation kernels (distance, prominence, clustering) but had zero
coverage of the graphics layer. Fixed via WS6.6 in the testing roadmap
(`roadmap_ws6_testing_ci_regression.md`, "Canvas rendering performance kernel (#240)"): five new
`--interactive-script` commands (`render`, `bulk-node-size`, `bulk-edge-color`, `move`, `quit`)
driving the real GUI binary under `QT_QPA_PLATFORM=offscreen`, with `scripts/run_render_perf_bench.sh`
comparing timing against upper-bound thresholds. This was a deliberate deviation from the
originally-sketched headless-CLI-kernel approach — see WS6.6 for why. This was the prerequisite for
the Performance Checklist items below; they remain unstarted.

## What Remains Open

### Performance Checklist

These are known, evidence-based gaps — not speculative wishlist items. Each is grounded in either
existing documentation or code already read. They haven't been scoped to Phase-1-style depth
(concrete approach + completion criteria) yet.

- **Node-selection hot path is still synchronous and unbatched.** Documented in
  `README_DEVELOPER_NOTES.md` ("Known hot path: node selection"):
  `GraphicsNode::itemChange(ItemSelectedHasChanged)` calls `setSize()` and `setColor()`, both of
  which call `prepareGeometryChange()` and trigger `adjust()` on every connected edge. For N
  selected nodes this is O(N × avg degree) geometry work, synchronous on the main thread, with no
  batching across the N nodes of a single rubber-band selection. Candidate direction: defer/coalesce
  geometry-change notifications across a whole selection event instead of firing per-node.

- **Bulk style operations still fire one `prepareGeometryChange()` per item.** `setEdgeArrowSize()`
  and similar bulk setters loop over every edge calling individual per-item geometry-change/update
  calls rather than batching.

- **Large-network interactive performance beyond `NoIndex`.** The scene-index-method fix
  (defaulting to `NoIndex`, per `README_DEVELOPER_NOTES.md`) addressed BSP-tree rebuild cost, but
  there's been no investigation of item culling / level-of-detail rendering for networks large
  enough that most nodes are off-screen or sub-pixel at the current zoom level — `QGraphicsView`
  still walks and paints every item in `NoIndex` mode regardless of visibility.

- **`GraphicsEdge` bezier/arrow path caching cost at scale** has not been profiled independently
  of the fixes above — worth revisiting once the render-perf kernel exists to measure it properly
  instead of guessing.

**Before picking any of these up:** if the canvas still feels slow, the most useful next step is
probably naming *which* interaction feels slow (initial load/paint of a large network? panning?
dragging a selection? zooming?) — that determines which item above is actually worth doing first,
rather than guessing.

### Feature Checklist

New `GraphicsWidget` capabilities, not performance-motivated. Not yet scoped to implementation-ready
depth.

- **#22 — Add text anywhere on the canvas.** A free-floating text annotation feature, independent
  of node/edge labels — e.g. for titling a network view or annotating a region. Needs a new
  `QGraphicsItem` type (or reuse of `GraphicsNodeLabel`'s text-rendering machinery without the
  node attachment), placement/editing UX, and a decision on whether it's persisted in saved network
  files (and if so, in which formats) or session-only, like guides.

## Work Rules

**Before committing any change described in this file:** run
`./scripts/run_golden_compares.sh`. All golden JSON baselines must still pass.
