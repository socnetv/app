# Architecture & Performance Roadmap (WS3)

> **Before committing any change described in this roadmap:** run
> `./scripts/run_golden_compares.sh`. All golden JSON baselines must still pass — this applies to
> every phase, group, and item below, not just the ones that call it out explicitly.

## Goal

Introduce a domain model that is independent from UI concerns and can be tested headlessly.
`Graph` currently mixes storage, algorithm state, caches, and UI signaling in one façade class;
the goal is to separate those concerns without a disruptive rewrite — see "Target Direction" below.

## Status at a Glance

| Milestone | Status | Detail |
|---|---|---|
| M1 — DistanceEngine parallelization (`PerSourceScratch` + parallel source loop) | ✅ Done (v3.6), 2.7×–8.3× speedup | [Archive](#m1--distanceengine-parallelization--complete) |
| M1 continuation — flat relation-keyed matrices | 🔵 Delegated to WS5 | [Active / Next Up](#m1-continuation--replace-distributed-vertex-qhash-storage-with-flat-relation-keyed-matrices) |
| #254 — GUI freeze during long weighted-centrality computation | 🔵 Scoped, not started | [Active / Next Up](#254--improve-ui-responsiveness-during-long-weighted-centrality-computations) |
| M2 — Introduce `GraphModel` | 🟡 In design | [M2](#m2--introduce-graphmodel) |
| M3 — Move pure data containers out of UI/Qt dependencies | ⚪ Not scoped (blocked on M2) | [M3](#m3--move-pure-data-containers-out-of-uiqt-dependencies) |
| M4 — Relocate caches into explicit cache objects | ⚪ Not scoped (blocked on M2) | [M4](#m4--gradually-relocate-caches-into-explicit-cache-objects) |
| GraphicsWidget canvas rendering performance | ✅ Phase 1 done, more scoped | Elevated to its own workstream — see [WS10](roadmap_graphicswidget_overhaul.md) |

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

## Active / Next Up

### M1 continuation — Replace distributed vertex QHash storage with flat relation-keyed matrices

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

Genuinely delegated (both roadmaps cross-reference each other, with a concrete target state and
milestone `A6` on the WS5 side) but not yet started — WS5's roadmap file is explicitly a "Skeleton."

### #254 — Improve UI responsiveness during long weighted-centrality computations

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

## M2 — Introduce `GraphModel`

**Status: in design.** Placeholder pending the research/design pass — see the note in Status at a
Glance. Will be expanded to the same depth as Phase 1/2 above (goal, current-reality specifics
with file/line references, concrete approach, completion criteria) before any code is touched.

## M3 — Move pure data containers out of UI/Qt dependencies

**Status: not yet scoped.** Depends on M2's shape — `GraphModel`'s adapter boundary determines
which containers can move and where they'd move to. Will be scoped once M2 is designed.

## M4 — Gradually relocate caches into explicit cache objects

**Status: not yet scoped.** Same dependency as M3 — needs M2's shape decided first.

---

## Shipped Work (Archive)

### M1 — DistanceEngine Parallelization ✅ Complete

#### Why the SSSP loop is the bottleneck

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

#### Phase 1 — Introduce `PerSourceScratch` ✅ Complete

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

#### Phase 2 — Parallel source loop ✅ Complete (`11da8ef`)

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

### Near-term DistanceEngine Deliverables (3.7) — Shipped

These features surface capabilities directly enabled by Phase 2 parallelisation, or improve
performance in related algorithm slices. They did not require domain model changes — they landed
as self-contained additions validated by the WS6 harness.

#### #89 — Distribution of geodesics by path length ✅ Done

`Graph::writeGeodesicDistribution()` + `Graph::graphGeodesicDistanceDistribution()` added in
`src/graph/distances/graph_distance_facade.cpp`. New **Analyze → Cohesion → Geodesic
Distribution** action (Ctrl+G,I) and matching Control Panel combo entry. Computation is
cache-aware: reuses `calculatedDistances` result when available.

#### #139 — Geodesic distance for specific node pairs ✅ Done

`Graph::graphGeodesicShortestPath()` added. The Distance dialog now shows the full node
sequence of the shortest path (BFS for unweighted, Dijkstra for weighted) in addition to the
distance value. The path edges are simultaneously **selected on the canvas** via
`GraphicsWidget::selectPath()` — using the normal Qt selection mechanism so move, inspect,
and context-menu operations work on the whole path out of the box. Only edges are selected
(not nodes) to avoid highlighting unrelated connected edges.

#### #64 — Clique Census performance ✅ Done

Tomita et al. (2006) pivot selection applied to `Graph::graphCliques()`. Pivot $ u \in P \cup X $
chosen to maximise $ |N(u) \cap P| $; main loop iterates only $ P \setminus N(u) $. Correctness
argument and paper references in the method docstring.

#### #249 — Viewport auto-fit and resize debouncing ✅ Done

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

### GraphicsWidget — Performance and Code Quality Overhaul (#250) ✅ Complete

Split into its own file: [`roadmap_graphicswidget_overhaul.md`](roadmap_graphicswidget_overhaul.md).
Separate track from the domain-model work above — canvas rendering, not `Graph`/`GraphVertex`.
Groups A, B, C, and the Final Gate (full Doxygen pass + dead-code sweep) all shipped; issue #250
is closed.

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
