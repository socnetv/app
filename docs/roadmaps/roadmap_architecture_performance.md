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

#### Group B — Hot-path allocation and scene-scan reductions

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

#### Group C — Structural changes (plan each individually before starting)

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
