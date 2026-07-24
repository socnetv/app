# Canvas Rendering Performance (WS10)

## Status

**Phase 1 — GraphicsWidget Performance and Code Quality Overhaul (#250): ✅ Complete.**
All planned work shipped: Group A (correctness fixes), Group B (hot-path allocation/scan
reductions), Group C (structural changes), and the Final Gate (documentation pass + dead-code
sweep). Issue [#250](https://github.com/socnetv/app/issues/250) is closed.

**Phase 2 onward:** see "Future Work" below — not yet started.

This was originally tracked as a subsection of the [Architecture & Performance Roadmap
(WS3)](roadmap_architecture_performance.md); split out into its own workstream (WS10) once #250
completed, since canvas rendering performance is a structurally separate, ongoing concern from
WS3's domain-model work (`Graph`/`GraphVertex`) — not a one-off cleanup with a defined end.

> **Before committing any change described in this file:** run
> `./scripts/run_golden_compares.sh`. All golden JSON baselines must still pass.

## Scope

`GraphicsWidget` (`src/graphicswidget.h`/`.cpp`) is the `QGraphicsView` subclass that owns the
canvas — node/edge scene items, selection, zoom/rotation, guides. This overhaul covered
correctness bugs, hot-path allocation/scan reductions, and structural changes (integer edge keys,
signal-chain hardening, dead-code removal), closing with a full documentation pass.

**What this overhaul did *not* do:** none of Groups A–C changed the actual paint/geometry cost of
rendering a large network — they removed waste (redundant allocations, redundant scans, redundant
transform applications) without touching the underlying per-item rendering model. If the canvas
still feels slow on large networks, that's expected — the real rendering-cost work is listed below,
not yet started.

## Future Work — Not Yet Started

These are known, evidence-based gaps — not speculative wishlist items. Each is grounded in either
existing documentation or code already read during this overhaul; none has been scoped to
Phase-1-style depth (concrete approach + completion criteria) yet.

- **No automated rendering-performance regression coverage.** The golden harness covers
  computation kernels (distance, prominence, clustering) but has zero coverage of the graphics
  layer — a regression in paint/geometry cost would currently go undetected. Already identified as
  WS6.6 in the testing roadmap (`roadmap_testing_ci_regression.md`, "Canvas rendering performance
  kernel (#240)"): a headless `kernel_render_perf_v8`-style CLI kernel driving a fixed
  render/bulk-update/drag sequence against a reference network, with timing-upper-bound baselines.
  This should probably come *before* further optimization work below, so improvements (and
  regressions) are actually measurable instead of judged by feel.

- **Node-selection hot path is still synchronous and unbatched.** Documented in
  `README_DEVELOPER_NOTES.md` ("Known hot path: node selection"):
  `GraphicsNode::itemChange(ItemSelectedHasChanged)` calls `setSize()` and `setColor()`, both of
  which call `prepareGeometryChange()` and trigger `adjust()` on every connected edge. For N
  selected nodes this is O(N × avg degree) geometry work, synchronous on the main thread, with no
  batching across the N nodes of a single rubber-band selection. Candidate direction: defer/coalesce
  geometry-change notifications across a whole selection event instead of firing per-node.

- **Bulk style operations still fire one `prepareGeometryChange()` per item.** `setEdgeArrowSize()`
  and similar bulk setters loop over every edge calling individual per-item geometry-change/update
  calls rather than batching. (`setNodeSizeAll()`, previously the other example here, turned out to
  be dead code and was deleted in the Final Gate — `setEdgeArrowSize` is the live remaining case.)

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

## Phase 1 — GraphicsWidget Performance and Code Quality Overhaul (#250) ✅ Complete

> **Process that was followed for every item below** (kept here as a record, not an instruction —
> all groups and the final gate are done): read the full method, its callers, and every signal/slot
> connection it participated in before touching it. Several items looked mechanical but carried
> non-obvious consequences — Qt object-ownership rules, cross-thread signal ordering, virtual
> dispatch, and implicit sharing semantics all turned at least one "simple rename" into a subtle
> bug along the way (see #C3's rotate-button regression). For each item: (1) map the full call
> graph, (2) check for override/virtual implications, (3) implement and document, (4) run
> `./scripts/run_golden_compares.sh` before moving to the next group.
>
> **Rules that were applied to every item:**
> - Every change is reflected in the method's Doxygen `/** @brief … */` block.
> - Obsolete methods confirmed to have no callers (verified with `grep -rn` across all of `src/`)
>   were deleted outright, with the removal documented in the commit message.
> - All golden JSON baselines passed after each group.
>
> **Final gate for the whole section:** every `GraphicsWidget` method — constructor, destructor,
> all public/protected/private methods, all slots, all signals — carries an accurate Doxygen block.
> ✅ Verified — see the Final Gate entry below.

## Group A — Correctness fixes and mechanical wins

- [x] **#A1 — Double-free / UB in `removeAllItems`** (`graphicswidget.cpp` lines 1423–1427) ✅ Done
  Superseded rather than patched: `removeAllItems()` was deleted entirely in Group B (#B3), replaced
  by the maintained `m_guides` list. The double-free can no longer occur because the method it was
  in no longer exists.

- [x] **#A2 — `contains()` + `value()` double hash lookup at 15+ sites** ✅ Done
  All originally-listed methods converted to single-probe `value(key, nullptr)` lookups in the
  first Group A pass. A final-gate audit (2026-07-24) found one instance that slipped through
  because it wasn't on the original site list: `removeNode()` (`graphicswidget.cpp` line 505) had
  the same `contains()`+`value()` pattern, plus three ungated `qDebug()` calls (a #B2-shaped issue
  in the same method). Fixed: single `value(nodeNum, nullptr)` lookup, `qDebug()` → `qCDebug(lcGW)`,
  and the post-delete self-check removed (guaranteed by construction, not a real branch).

- [x] **#A3 — By-value argument copies** (`graphicswidget.cpp` line 980 and line 958) ✅ Done
  `setSelectedNodes` takes `const QList<int> &`. `hasNode` is moot — deleted outright in #C2
  (zero callers anywhere in the tree).

- [x] **#A4 — `setEdgeOffsetFromNode` rebuilds edge name manually** (`graphicswidget.cpp` lines 1161–1162) ✅ Done
  Now calls the shared `edgeKey(source, target)` (renamed from `createEdgeName` in #C1) instead of
  duplicating the key construction inline.

## Group B — Hot-path allocation and scene-scan reductions

- [x] **#B1 — `handleSelectionChanged` calls `scene()->selectedItems()` twice** (lines 1474–1521) ✅ Done
  Calls `scene()->selectedItems()` once, iterates the result once to populate both
  `m_selectedNodes` and `m_selectedEdges`, then emits `userSelectedItems`.

- [x] **#B2 — `qDebug` in hot paths not guarded for release builds** (multiple locations) ✅ Done
  `Q_LOGGING_CATEGORY(lcGW, "socnetv.graphicswidget")` introduced; all originally-listed hot-path
  call sites (`wheelEvent`, `mousePressEvent`, `mouseReleaseEvent`, `zoomIn`/`zoomOut`,
  `changeMatrixScale`) converted to `qCDebug(lcGW)`. Remaining plain `qDebug()` calls are all in
  one-shot / low-frequency contexts (constructor, `clear()`, `setRelation`, double-click, one-time
  teardown) and were left as-is — except `removeNode()`, fixed alongside #A2 above (see note there).

- [x] **#B3 — `scene()->items()` full-scene scan in `setAllItemsVisibility` / `removeAllItems`**
  (lines 1377–1430; `clearGuides` → `removeAllItems(TypeGuide)`) ✅ Done
  `QList<GraphicsGuide*> m_guides` added; `addGuideCircle()`/`addGuideHLine()` append to it,
  `clearGuides()` iterates and deletes from it directly with no `scene()->items()` scan.
  `removeAllItems()` itself was deleted as dead code once nothing else called it.

- [x] **#B4 — `selectAll` uses viewport pixels as scene coordinates** (`graphicswidget.cpp` lines 1449–1452) ✅ Done
  Now uses `QPainterPath` + `scene()->sceneRect()` instead of viewport pixel dimensions. The
  `selectedItems().size()` log line was wrapped in `qCDebug(lcGW)` rather than removed.

- [x] **#B5 — `mouseReleaseEvent` allocates `selectedItems()` on every node mouse-up** (lines 1690–1696) ✅ Done
  Took Option B (minimal): reuses the `m_selectedNodes` member kept in sync by
  `handleSelectionChanged()` instead of re-querying the scene.

## Group C — Structural changes (plan each individually before starting)

- [x] **#C1 — `createEdgeName` QString allocations → integer edge key** (`graphicswidget.cpp` line 178) ✅ Done
  `createEdgeName` built `"relation:v1>v2"` with several heap allocations per call, hit on every
  edge operation. Replaced with `GraphicsWidget::edgeKey()`, returning a decimal-digit-packed
  `quint64` instead of a bit-packed one (chosen for debuggability — the digits read left to right
  as relation/v1/v2, same order as the old string):
  `key = quint64(relation) * 10^16 + quint64(v1) * 10^8 + quint64(v2)`
  — a 3-digit relation field (0-999) and two 8-digit node-number fields (0-99,999,999 each).
  This is positional encoding, not a mixing hash: collisions between distinct (relation, v1, v2)
  triples are structurally impossible as long as each field stays under its digit budget, backed
  by three `Q_ASSERT_X` checks in `edgeKey()`. `H_StrToEdge` renamed to
  `H_KeyToEdge` (`QHash<quint64, GraphicsEdge*>`); member `edgeName` renamed to `m_edgeKey`; the
  local `reverseEdgeName` (`QString`) renamed to `reverseEdgeKey` (`quint64`). Explanatory
  comments in `graph_edges.cpp` referencing the old function name updated to match.
  All golden regression baselines pass unchanged.

- [x] **#C2 — `hasNode` O(N) loop with repeated `toInt()`** (`graphicswidget.cpp` lines 958–970) ✅ Done
  Turned out to be moot: `grep -rn "hasNode(" src/` found zero callers anywhere in the tree,
  including `mainwindow.cpp`. There was no O(N) cost to fix because the method was never invoked —
  deleted outright (declaration + definition) per the final-gate dead-code rule instead of
  refactoring dead code.

- [x] **#C3 — `zoomToFit` / `reset` may double-apply transform via slider signal chain** (lines 1929–1946) ✅ Done
  Audited both chains: `zoomSlider::sliderMoved` (the #249 fix) only fires on user drag, not on
  programmatic `setValue()`, so the zoom chain was already safe — no change needed there.
  `rotateSlider::valueChanged`, unlike `sliderMoved`, **does** fire on programmatic `setValue()`,
  so `rotateLeft()`/`rotateRight()` bounced every click through `changeMatrixRotation()` twice
  (harmless — idempotent — but wasteful). Fixed by converting the
  `rotationChanged -> rotateSlider::setValue` connection in `mainwindow.cpp` to a lambda wrapped
  in `QSignalBlocker`, matching option (a) from the original plan.
  **Follow-up correction:** the first version of this fix broke the rotate buttons — unlike
  `zoomIn`/`zoomOut`, `rotateLeft`/`rotateRight` never called `changeMatrixRotation()` directly;
  the transform was only ever applied as a side effect of the (now-blocked) slider's
  `valueChanged`. Fixed by making `rotateLeft`/`rotateRight` call `changeMatrixRotation()`
  directly, same pattern as the zoom methods, so the button path no longer depends on the
  slider's signal chain at all.

- [x] **#C4 — `edgesHash.reserve(500000)` pre-allocated at startup regardless of graph size**
  (`graphicswidget.cpp` line 70) ✅ Done
  A "reserve based on real counts once known" fix was considered but dropped: `Graph::signalGraphLoaded`
  only fires *after* all `createNode`/`createEdge` calls have already populated the hashes, so it
  can't help size the load that's already happening — only some hypothetical next one. Given
  `QHash`'s amortized-O(1) growth means a fixed reserve saves only a handful of cheap rehashes on
  very large loads, while costing every small/medium network (the overwhelming common case) a
  bucket array sized for 500,000 entries at construction, the two `reserve()` calls were removed
  outright rather than replaced with load-time sizing.

## Final gate — documentation and dead-code removal ✅ Done

- [x] **Documentation pass:** audited all 78 methods in `graphicswidget.cpp` (constructor,
  destructor, 76 others). 76 already had accurate `@brief` blocks from the Group A/B/C work.
  Two real gaps found and fixed:
  - `setEdgesBezier()` had no doc block at all — added one.
  - `reset()`'s doc block (`"Resets to default rotation, zoom and scale"`) was orphaned two
    comment-blocks above `zoomToFit()` instead of sitting above `reset()` itself — likely
    stranded there when `zoomToFit()`'s own large doc block was added during the #253 fix.
    Moved to the correct location.
  Separately, all 13 signals in `graphicswidget.h` had zero documentation anywhere (no `.cpp`
  body exists for signals, so the file's usual "doc lives above the definition" convention
  doesn't apply) — added inline `@brief` blocks directly above each signal declaration.

- [x] **Dead-code removal:** ran a systematic zero-caller sweep (`grep -rn` per method name across
  all of `src/`) across all 78 methods, beyond the specific items already caught by #C2 (`hasNode`)
  earlier. Found one more: `setNodeSizeAll()` had no callers anywhere — only a mention by name in
  the WS6 roadmap (`roadmap_testing_ci_regression.md`, #240 section) as an example of an unbatched
  bulk operation. Deleted the method (declaration + definition) and removed it from that roadmap
  example list, since `setEdgeArrowSize` (confirmed alive, called from `mainwindow.cpp`) already
  covers the point being made there.
  Also removed one line of dead code spotted along the way: a commented-out obsolete
  `userSelectedItems` signal signature in `graphicswidget.h`, superseded by the real declaration
  immediately below it.
  All golden regression baselines pass unchanged.
