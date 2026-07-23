# Architecture & Performance Roadmap (WS3)

## Goal

Introduce a domain model that is independent from UI concerns and can be tested headlessly.
The first concrete step — extracting per-source algorithm scratch state from `Graph` and
`GraphVertex` into `PerSourceScratch` and parallelising the `DistanceEngine` source loop —
is complete (shipped in v3.6, 2.7×–8.3× speedup). The roadmap continues with near-term
DistanceEngine feature deliverables and then the longer-arc domain model milestones M2–M4.

## Current Reality

- `Graph` mixes storage, algorithm state, caches, and UI signaling.
- `GraphVertex` acts as both node storage and analysis result cache.
- Per-source SSSP scratch has been extracted into `PerSourceScratch` (Phase 1) and the
  source loop now runs in parallel via `QtConcurrent` (Phase 2). The remaining mixed
  concerns in `Graph`/`GraphVertex` are targeted by M2–M4 and WS5.

## Target Direction

- Separate "model" (nodes/edges/relations) from "services/algorithms".
- Keep `Graph` as façade during transition.
- Algorithm scratch state belongs in the algorithm, not in the domain objects.

---

## First Execution — DistanceEngine Parallelization

### Why the SSSP loop is the bottleneck

`DistanceEngine::runAllSources()` calls `bfsSSSP` or `dijkstraSSSP` once per source vertex
(V calls total). Each call traverses all edges reachable from that source — O(V + E) work.
Total sequential cost: **O(V × (V + E))**.

For a network with V vertices and E edges, the dominant cost is V × E edge traversals.
Example: V=5 000, E=50 000 → 250 million edge traversals, all sequential before Phase 2.

Expected parallel speedup (P cores, edge-dominated workloads):

| V      | E (avg degree) | Sequential (est.) | 8 cores  | 16 cores |
|--------|---------------|-------------------|----------|----------|
| 1 000  | 5 000 (k=5)   | ~1 s              | ~150 ms  | ~80 ms   |
| 2 000  | 20 000 (k=10) | ~8 s              | ~1.1 s   | ~600 ms  |
| 5 000  | 50 000 (k=10) | ~50 s             | ~7 s     | ~4 s     |
| 10 000 | 200 000 (k=20)| ~15 min           | ~2 min   | ~1 min   |

Speedup is near-linear with core count because sources are fully independent once per-source
scratch state is local. Synchronisation cost (BC accumulation) is O(V × threads), negligible
against O(V × E) edge traversals.

---

### Phase 1 — Introduce `PerSourceScratch` ✅ Complete

**Goal:** Make the per-source scratch state moveable without changing any algorithm logic.

Create `src/engine/per_source_scratch.h`:

```cpp
struct PerSourceScratch {
    std::stack<int>     Stack;         // Brandes traversal stack
    QVector<QList<int>> Ps;            // predecessor lists, indexed by vertex position
    QVector<qreal>      delta;         // dependency accumulators, indexed by vertex position
    QVector<qreal>      dist;          // BFS-local distance from s, indexed by vertex position
    QVector<int>        sigma;         // shortest-path counts, indexed by vertex position
    QHash<qreal, int>   nthOrder;      // nth-order neighbourhood sizes
    int                 componentSize; // reachable component size

    void allocate(int totalVertices);        // resize all containers once before the source loop
    void resetPerSource(bool computeCentralities); // reset per-source state before each source
};
```

Thread `PerSourceScratch&` through `bfsSSSP` and `dijkstraSSSP`. Replace every
Graph/GraphVertex scratch call with the struct member:

| Current call | Replacement |
|---|---|
| `graph.ssspStackPush/Pop/Clear/...` | `scratch.Stack.*` |
| `(*it)->clearPs()` + `appendToPs(u)` | `scratch.Ps[vi].clear()` / `.append(u)` |
| `vertex->delta()` / `setDelta(...)` | `scratch.delta[vi]` |
| `graph.ssspNthOrderClear/Increment/...` | `scratch.nthOrder.*` |
| `graph.ssspComponentReset/Add/Size` | `scratch.componentSize` |
| `vertex->setDistance(w, d)` *(BFS-local)* | `scratch.dist[wi]` |
| `vertex->setShortestPaths(w, sp)` *(BFS-local)* | `scratch.sigma[wi]` |

At the end of each source, an explicit write-back step copies `scratch.dist` and
`scratch.sigma` into the per-vertex `m_distance` / `m_shortestPaths` hashes (same semantics
as today, just made explicit). The public vertex API is unchanged.

**Completion criteria:**
- `run_golden_compares.sh` passes
- `run_benchmarks.sh` shows no regression
- Dead members (`Graph::Stack`, `Graph::sizeOfNthOrderNeighborhood`, `Graph::sizeOfComponent`,
  `GraphVertex::myPs`, `GraphVertex::m_delta`) removed in Phase 2 dead-code cleanup ✅

---

### Phase 2 — Parallel source loop ✅ Complete (`11da8ef`)

**Goal:** Run bfsSSSP / dijkstraSSSP concurrently across sources using `QtConcurrent::blockingMap`.

**Approach (as shipped):**

Each worker thread owns a `ThreadLocalState` (`src/engine/thread_local_state.h`) containing:
- A `PerSourceScratch` reused across all sources that thread processes.
- `partialBC[vi]` / `partialSC[vi]` per-vertex accumulator arrays (no mutex needed — each
  thread owns its slice exclusively during the parallel loop).
- Running totals for graph-wide aggregates (`totalDistanceSum`, `totalGeodesicsCount`,
  `maxDiameter`, `totalSumPC`, `totalSumSPC`).

Slot assignment uses a `QMutex`-protected `QHash<Qt::HANDLE, int>` re-created each
`runAllSources()` call — avoids a stale-slot data race that `static thread_local` would
cause on repeated benchmark calls.

**APSP write-back (`m_distance`, `m_shortestPaths`):** race-free without any mutex —
`si` is unique per thread, so each thread writes to disjoint vertex rows. The per-vertex
mutex array originally planned in the roadmap was not needed.

**Graph-level aggregates:** accumulated into `ThreadLocalState` fields during traversal;
merged into graph state in a single-threaded reduction step after `blockingMap` returns
(no mutex needed in the reduction step either).

**Progress updates:** `std::atomic<int> nextSlot` counter polled via
`sink.progressUpdate(nextSlot.loadRelaxed())` from the coordinating thread — no per-source
signal emission inside the parallel loop.

**Thread count:** `QThread::idealThreadCount()` at call time. Settings-dialog exposure
deferred to a later phase.

**Dead code removal** ✅ Done — removed in the Phase 2 dead-code cleanup:
- `Graph::Stack`, `Graph::sizeOfNthOrderNeighborhood`, `Graph::sizeOfComponent`
  (and all `ssspStack*`, `ssspNthOrder*`, `ssspComponent*` accessors on `Graph`)
- `GraphVertex::myPs` + `clearPs()` + `appendToPs()`
- `GraphVertex::m_delta` + `delta()` + `setDelta()`

**Benchmark results (Debug build, 24-core Linux):**

| Case | Before Phase 2 | After Phase 2 | Speedup |
|---|---|---|---|
| BA500 N=500 E=1219 C1 | 679 ms | 255 ms | **2.7×** |
| DIST 1000N/10000A C0 | 28 423 ms | 3 431 ms | **8.3×** |
| DIST 1000N/10000A C1 | 47 020 ms | 5 949 ms | **7.9×** |

All 36 golden JSON baselines pass.

---

### Phase 3 — Replace distributed vertex QHash storage with flat relation-keyed matrices

**Delegated to WS5.** See [`roadmap_matrices_modernization.md`](roadmap_matrices_modernization.md).

Summary: replace `GraphVertex::m_distance` and `GraphVertex::m_shortestPaths` (per-vertex
QHash stores, keyed by target vertex and relation) with a centralised `QHash<int, Matrix>`
on the `Graph` object (keyed by relation), where `Matrix` is the existing SocNetV matrix class.

Benefits:
- Eliminates per-vertex QHash lookup overhead inside the back-propagation inner loop
  (`shortestPaths(v1)` called per predecessor per vertex per source)
- Makes APSP write-back in Phase 2 a flat array write: `distMatrix[rel][si * N + ti] = d`
- Removes the per-vertex mutex array introduced in Phase 2 (each source owns its own row —
  no contention at all)
- Aligns with WS5 goals (cancellable, testable matrix subsystem)

---

## Near-term DistanceEngine Deliverables (3.7)

These features surface capabilities directly enabled by Phase 2 parallelisation, or improve
performance in related algorithm slices. They do not require domain model changes — they land
as self-contained additions validated by the WS6 harness.

### #89 — Distribution of geodesics by path length ✅ Done

`Graph::writeGeodesicDistribution()` + `Graph::graphGeodesicDistanceDistribution()` added in
`src/graph/distances/graph_distance_facade.cpp`. New **Analyze → Cohesion → Geodesic
Distribution** action (Ctrl+G,I) and matching Control Panel combo entry. Computation is
cache-aware: reuses `calculatedDistances` result when available.

### #139 — Geodesic distance for specific node pairs ✅ Done

`Graph::graphGeodesicShortestPath()` added. The Distance dialog now shows the full node
sequence of the shortest path (BFS for unweighted, Dijkstra for weighted) in addition to the
distance value. The path edges are simultaneously **selected on the canvas** via
`GraphicsWidget::selectPath()` — using the normal Qt selection mechanism so move, inspect,
and context-menu operations work on the whole path out of the box. Only edges are selected
(not nodes) to avoid highlighting unrelated connected edges.

### #64 — Clique Census performance ✅ Done

Tomita et al. (2006) pivot selection applied to `Graph::graphCliques()`. Pivot $ u \in P \cup X $
chosen to maximise $ |N(u) \cap P| $; main loop iterates only $ P \setminus N(u) $. Correctness
argument and paper references in the method docstring.

### #249 — Viewport auto-fit and resize debouncing ✅ Done

**Problem:** `resizeEvent` fired O(N × fps) cross-thread rescaling signals during a window-drag
resize. Zoom buttons, mouse wheel, and `reset()` relied on the `zoomSlider::valueChanged →
changeMatrixScale` chain — a no-op when the slider value was unchanged. After resize, the network
often vanished from view because `canvasSizeSet` never triggered a viewport re-fit.

**Changes:**
- `resizeEvent` now debounces via a 150 ms single-shot `QTimer`; one `canvasSizeSet` fires per
  stable stop instead of once per pixel of window drag. Old guide-repositioning loop (O(items))
  replaced by `clearGuides()` — guides are recreated by the next layout run anyway.
- `canvasSizeSet` emits `signalLayoutFinished` after all `setNodePos` signals, so the viewport
  auto-fits as soon as the main thread finishes moving nodes.
- Zoom slider wired to `sliderMoved` (genuine user drag only). `zoomIn()`, `zoomOut()`, and
  `reset()` now call `changeMatrixScale()` directly, decoupling view updates from the slider chain
  and making Ctrl+wheel, button auto-repeat, and keyboard shortcuts reliable at any slider position.
- `zoomToFit()` caps the computed zoom index at `m_zoomIndexInit` (100 %) — small networks are
  never over-zoomed; only layouts larger than the viewport scale down.

**Follow-up fix (#253):** the exact-fit-with-margin calculation shrank content that was only
marginally larger than the viewport (a few percent, from node marker/label overhang beyond the
raw coordinate range) — e.g. a 1000-node Erdős–Rényi network or `geom.net` on load ended up
visibly smaller than the viewport, needing a manual Ctrl+0 to fill the canvas correctly.
`zoomToFit()` now snaps to the initial 100 % level whenever the computed fit scale is within 75 %
of it, matching `reset()`'s tight, margin-free look; only content that would need at least a
quarter of its size cut still triggers a real zoom-out.

### #254 — Improve UI responsiveness during long weighted-centrality computations (pending)

**Problem:** computing a weighted, inverted-weight centrality index (e.g. Betweenness Centrality)
on a large network makes the whole application completely unresponsive — not just slow, but
unable to repaint, receive input, or even respond to window-manager focus/switch requests — for
the entire duration of the computation. Confirmed identically present in v3.6 (not a regression):
for `geom.net` (7343 nodes, weighted, inverted, BC), the release build showed 795–900 % CPU across
~8 cores (`ps` state `R`, genuinely computing, not deadlocked) and eventually completed the
layout, but only after several minutes with the window completely frozen throughout.

**Root cause:** `MainWindow::slotLayoutXByProminenceIndex()` calls `activeGraph-
>layoutByProminenceIndex(...)` as a direct, synchronous C++ call from the GUI thread. A direct
method call always executes on the caller's thread regardless of the callee `QObject`'s thread
affinity — `Graph` being `moveToThread(&graphThread)`'d doesn't help here. For indices without a
dedicated branch (including BC), this falls through to `DistanceEngine::compute()`
(`src/engine/distance_engine.cpp`), which uses `QtConcurrent::blockingMap(sources, ...)` to
parallelize per-source Dijkstra/BFS runs across the global thread pool — but `blockingMap` blocks
the *calling* thread (here, the GUI thread) until every worker finishes. The computation is
genuinely parallelized, but the GUI thread never returns to its event loop for the whole duration.

**Secondary finding:** `distance_engine.cpp` has ~75 unconditional `qDebug()` calls, several
inside the per-edge-relaxation inner loop (fires on the order of the total edge-relaxation count
— potentially billions for a graph this size). Unlike `qCDebug(category)`, plain `qDebug()` isn't
cheaply short-circuited by a disabled logging-category filter rule — the stream
construction/formatting cost is paid on every call regardless of whether output is enabled. Worth
converting to a dedicated category (same pattern as `lcGW` in `GraphicsWidget`), independent of
the main-thread-blocking fix.

**UX note (from live testing):** the app's existing progress-dialog mechanism
(`progressCreate`/`progressUpdate`/`progressFinish`) is disabled by default because it makes
large computations *slower* — each `progressUpdate()` call crosses a signal/slot boundary and
triggers a repaint, which adds real overhead when called millions/billions of times. A fix here
should **not** naively wire that granular mechanism into this hot path. Consider a separate,
coarse-grained "computation in progress" indicator (shown once, updated rarely if at all,
dismissed on completion) instead — decoupled from the actual fix, which is moving the blocking
wait off the GUI thread (e.g. `QFutureWatcher` + signal-based completion instead of
`blockingMap`).

See #254 for the full write-up and hints.

---

### GraphicsWidget — Performance and Code Quality Overhaul

> **Before touching any item below:** read the full method, its callers, and every signal/slot
> connection it participates in. Several items look mechanical but carry non-obvious consequences:
> Qt object-ownership rules, cross-thread signal ordering, virtual dispatch, and implicit sharing
> semantics can all turn a "simple rename" into a subtle bug. For each item: (1) map the full call
> graph, (2) check for override/virtual implications, (3) implement and document, (4) run
> `./scripts/run_golden_compares.sh` before moving to the next group.
>
> **Rules that apply to every item:**
> - Every change must be reflected in the method's Doxygen `/** @brief … */` block — update or
>   write one as part of the same commit.
> - Obsolete methods confirmed to have no callers (verified with `grep -rn` across all of `src/`)
>   may be deleted outright; document the removal in the commit message.
> - All 36 golden JSON baselines must still pass after each group.
>
> **Final gate for the whole section:** every `GraphicsWidget` method — constructor, destructor,
> all public/protected/private methods, all slots, all signals — carries an accurate Doxygen block.

#### Group A — Correctness fixes and mechanical wins

- [ ] **#A1 — Double-free / UB in `removeAllItems`** (`graphicswidget.cpp` lines 1423–1427)
  `guide->deleteLater()` posts a deferred delete, then `delete *item` immediately destroys the
  same pointer — undefined behaviour, crash under sanitizers. Pick one strategy: either
  `scene()->removeItem(guide); guide->deleteLater();` or `delete guide;`.
  Also add a null-check on the `qgraphicsitem_cast` result.
  _Before fixing:_ confirm whether `GraphicsGuide::die()` calls `deleteLater()` internally —
  that would make either choice safe on its own.

- [ ] **#A2 — `contains()` + `value()` double hash lookup at 15+ sites**
  Pattern `if (nodeHash.contains(k)) nodeHash.value(k)->...` probes the bucket twice.
  Replace with `if (auto *n = nodeHash.value(k, nullptr)) n->...` everywhere.
  Affected methods: `setNodeVisibility`, `setNodeSize`, `setNodeNumberColor/Size/Distance`,
  `setNodeLabelColor/Size/Distance`, `setEdgeLabel`, `setEdgeColor`, `setEdgeDirectionType`,
  `setEdgeWeight`, `removeEdge`, `setEdgeVisibility`.
  _Before fixing:_ confirm `QHash::value(key, defaultValue)` behaviour is identical to the
  contains+value pair for all edge cases (missing key, null stored value).

- [ ] **#A3 — By-value argument copies** (`graphicswidget.cpp` line 980 and line 958)
  `setSelectedNodes(QList<int> list)` → `setSelectedNodes(const QList<int> &list)`.
  `hasNode(QString text)` → `hasNode(const QString &text)`.
  _Before fixing:_ update the declarations in `graphicswidget.h` to match; check if any
  Qt signal connection passes a temporary that would be invalidated by a const-ref parameter
  (it would not — Qt copies arguments at the signal boundary for queued connections).

- [ ] **#A4 — `setEdgeOffsetFromNode` rebuilds edge name manually** (`graphicswidget.cpp` lines 1161–1162)
  Duplicates the 7-allocation string chain and reads `m_curRelation` instead of the `relation`
  parameter — latent lookup failure when the active relation differs from the edge's registered one.
  Replace with a call to `createEdgeName(source, target)`.
  _Before fixing:_ verify that `createEdgeName` falls back to `m_curRelation` when `relation == -1`,
  and that the callers of `setEdgeOffsetFromNode` always pass `relation == -1`.

#### Group B — Hot-path allocation and scene-scan reductions

- [ ] **#B1 — `handleSelectionChanged` calls `scene()->selectedItems()` twice** (lines 1474–1521)
  `selectedNodes()` and `selectedEdges()` each call `scene()->selectedItems()`, allocating a
  separate `QList` and traversing the scene index. During a rubber-band drag over N nodes this
  fires N times, two allocations each.
  Call `scene()->selectedItems()` once, store in `const QList<QGraphicsItem*> items`, iterate
  once to populate both `m_selectedNodes` and `m_selectedEdges`, then emit `userSelectedItems`.
  _Before fixing:_ audit all callers of the public `selectedNodes()` and `selectedEdges()` methods;
  if they are called externally they must still return accurate data (either from the cached
  members or by performing their own lookup).

- [ ] **#B2 — `qDebug` in hot paths not guarded for release builds** (multiple locations)
  `wheelEvent` (line 1727), `mousePressEvent` (lines 1587–1654), `mouseReleaseEvent`
  (lines 1687–1707), `zoomIn`/`zoomOut` (lines 1747, 1767), `changeMatrixScale` (lines 1790, 1820).
  Qt does not strip `qDebug` in release builds unless `QT_NO_DEBUG_OUTPUT` is defined.
  Introduce a named logging category (`Q_LOGGING_CATEGORY(lcGW, "socnetv.graphicswidget")`)
  and replace all calls with `qCDebug(lcGW)`. The category is disabled at runtime by default
  with no recompile needed.
  _Before fixing:_ check whether `QT_NO_DEBUG_OUTPUT` is already set in the release CMake
  profile; if it is, wrapping is still preferable because the category approach gives per-module
  control without a recompile.

- [ ] **#B3 — `scene()->items()` full-scene scan in `setAllItemsVisibility` / `removeAllItems`**
  (lines 1377–1430; `clearGuides` → `removeAllItems(TypeGuide)`)
  `scene()->items()` is O(N log N) over all scene items — nodes, edges, weight labels, numbers,
  labels, and guides — just to find the few guide objects. `clearGuides()` is called on every
  debounced resize.
  Maintain a `QList<GraphicsGuide*> m_guides` member in `graphicswidget.h`. Append in
  `addGuideCircle()` and `addGuideHLine()`; iterate and delete from `m_guides` directly inside
  `clearGuides()` without touching `scene()->items()`.
  _Before fixing:_ confirm that guide items are owned by the scene (added via `scene()->addItem()`);
  removing them from `m_guides` without also removing them from the scene would leak. The list
  management must mirror the scene management exactly.

- [ ] **#B4 — `selectAll` uses viewport pixels as scene coordinates** (`graphicswidget.cpp` lines 1449–1452)
  `path.addRect(0, 0, width(), height())` passes device-pixel dimensions to
  `scene()->setSelectionArea()`, which expects scene coordinates. After any pan, zoom, or
  rotation this selects the wrong region.
  Replace with `scene()->setSelectionArea(QRectF(scene()->sceneRect()))`.
  Also remove (or wrap in `qCDebug`) the `scene()->selectedItems().size()` call used only
  for a log line — it allocates a full item list just to count elements.
  _Before fixing:_ verify rubber-band selection (which goes through a separate `QGraphicsView`
  code path) is unaffected, then test `selectAll` after a zoom-in and after a pan.

- [ ] **#B5 — `mouseReleaseEvent` allocates `selectedItems()` on every node mouse-up** (lines 1690–1696)
  On every node mouse release, `scene()->selectedItems()` is called and iterated to emit one
  `userNodeMoved` signal per selected node. For a 500-node drag: 500 sequential cross-thread
  signal emissions inside the event handler.
  Option A (preferred): introduce a batch signal
  `userNodesMoved(const QList<QPair<int,QPointF>> &)` and update the `Graph` slot to match.
  Option B (minimal): use the `m_selectedNodes` member populated by `handleSelectionChanged`
  instead of re-querying the scene.
  _Before fixing:_ if changing the signal signature, update the connection in `mainwindow.cpp`
  and the receiving slot in `Graph`; run the full regression harness.

#### Group C — Structural changes (plan each individually before starting)

- [ ] **#C1 — `createEdgeName` QString allocations → integer edge key** (`graphicswidget.cpp` line 178)
  `createEdgeName` builds `"relation:v1>v2"` with 8+ heap allocations per call. It is called in
  every edge operation, up to 4 times per slot. `edgesHash` is a `QHash<QString, GraphicsEdge*>`.
  Replace the key with `quint64` packed as
  `(quint64(relation) << 40) | (quint64(v1) << 20) | quint64(v2)` (node numbers < 2²⁰ ≈ 1M;
  adjust shifts if wider ranges are needed). Change the `H_StrToEdge` typedef in
  `graphicswidget.h` to `QHash<quint64, GraphicsEdge*>`. Rename `createEdgeName` → `edgeKey`
  (returns `quint64`); rename the member `edgeName` → `m_edgeKey`.
  _Before fixing:_ audit every use of `edgeName` / `edgesHash` across the full file and any
  external callers; confirm actual node-number and relation-index ranges used in the codebase;
  check whether any code serialises or logs the edge key as a string (would need updating).

- [ ] **#C2 — `hasNode` O(N) loop with repeated `toInt()`** (`graphicswidget.cpp` lines 958–970)
  `text.toInt(&ok, 10)` is called once per node in the hash on every invocation.
  Refactor: convert once before any loop; for numeric input do a direct O(1) `nodeHash.find()`;
  fall back to O(N) label scan only when the input is non-numeric or the number is not found.
  _Before fixing:_ confirm all callers pass either a node-number string or a label; if labels can
  be purely numeric, the O(N) label fallback is mandatory even after a hash hit fails.

- [ ] **#C3 — `zoomToFit` / `reset` may double-apply transform via slider signal chain** (lines 1929–1946)
  Both call `changeMatrixScale()` directly, then `emit zoomChanged()`. If anything connects
  `valueChanged → changeMatrixScale` the transform is applied twice (two repaints).
  Currently mitigated by the `sliderMoved` change in #249, but the coupling is fragile.
  Fix: when updating the slider for display purposes, use `QSignalBlocker` to prevent re-entry.
  _Before fixing:_ `GraphicsWidget` holds no direct pointer to `zoomSlider` (it lives in
  `MainWindow`). Decide between: (a) emitting a separate display-only signal that MW connects
  to `slider->setValue` via a blocked connection, or (b) passing the slider pointer at
  construction time. Choose and document the pattern before writing any code.

- [ ] **#C4 — `edgesHash.reserve(500000)` pre-allocated at startup regardless of graph size**
  (`graphicswidget.cpp` line 70)
  Pre-allocates megabytes of hash bucket memory unconditionally. Remove the fixed reserve.
  Call `edgesHash.reserve(edgeCount)` and `nodeHash.reserve(nodeCount)` after the file header
  is parsed and actual counts are known — before `drawNode`/`drawEdge` calls begin.
  _Before fixing:_ identify where `Graph` signals vertex/edge counts (e.g. via
  `signalNodesFound` or a dedicated pre-draw signal) and wire a slot or direct call on
  `GraphicsWidget` to trigger the reserve at the right moment.

#### Final gate — documentation and dead-code removal

- [ ] **Documentation pass:** every `GraphicsWidget` method — constructor, destructor, all public,
  protected, and private methods, all slots, and all signals — must carry an accurate Doxygen
  `/** @brief … */` block. Methods that already have one must be reviewed against current behaviour
  and updated where stale.

- [ ] **Dead-code removal:** identify any methods with no external callers using
  `grep -rn "methodName" src/` across the full source tree and Qt Creator's "Find Usages".
  Confirm a method is unreachable before deleting it. Document each removal in the commit message.

---

## General Milestones (long-term WS3 arc)

- M1: Identify minimal model surface required by algorithms ✅ *(PerSourceScratch introduced, dead code removed, parallel loop shipped in v3.6)*
- M2: Introduce `GraphModel` (adapter over existing Graph internals initially)
- M3: Move pure data containers out of UI/Qt dependencies where possible
- M4: Gradually relocate caches into explicit cache objects

---

## Work Rules

- Prefer adapters/wrappers first, not data migrations.
- Every phase must be regression-clean before the next begins.
- Dead members are removed after the phase that makes them dead is validated — not before.

---

## Cross-cutting dependency — Undo/Redo (#31)

Structural undo (add/delete nodes, weight changes, attribute mutations on the canvas) requires
a `QUndoStack` integrated across the full Graph mutation API. This is a WS3 concern: a proper
command pattern can only be introduced cleanly once the domain model and mutation API are
stable. #31 is explicitly deferred until at least M2 of this roadmap.

Note: filter-level undo (non-destructive visibility operations) is already handled by the
`m_visibilityHistory` snapshot stack in `Graph` and is not blocked on WS3.
