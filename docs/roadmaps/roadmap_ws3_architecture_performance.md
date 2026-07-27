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
| #254 — GUI freeze during long weighted-centrality computation | ✅ Done (28/28 entry points) | [Archive](#254--ui-responsiveness-during-long-weighted-centrality-computations--complete) |
| M2 — Introduce `GraphModel` | 🟡 Design drafted, not started | [M2](#m2--introduce-graphmodel) |
| M3 — Move pure data containers out of UI/Qt dependencies | ⚪ Not scoped (blocked on M2) | [M3](#m3--move-pure-data-containers-out-of-uiqt-dependencies) |
| M4 — Relocate caches into explicit cache objects | ⚪ Not scoped (blocked on M2) | [M4](#m4--gradually-relocate-caches-into-explicit-cache-objects) |
| GraphicsWidget canvas rendering performance | ✅ Phase 1 done, more scoped | Elevated to its own workstream — see [WS10](roadmap_ws10_graphicswidget_overhaul.md) |

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

**Delegated to WS5.** See [`roadmap_ws5_matrices_modernization.md`](roadmap_ws5_matrices_modernization.md).

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

Genuinely delegated (both roadmaps cross-reference each other) — WS5's roadmap is now fully
scoped (milestone A2), but the migration itself is gated on an empirical validation step (A2.0):
a dense matrix should win on both speed and memory for connected graphs, but could lose on memory
for graphs with many disconnected components/isolates, where the QHash simply never stores an
entry for unreachable pairs. A2.0 measures this on real data before the migration proceeds, rather
than assuming the win.

## M2 — Introduce `GraphModel`

**Status: design drafted (2026-07-25), not yet started.**

**Every item below is filtered through WS3's actual purpose — performance and UX, not tidiness for
its own sake.** M2 is worth doing only insofar as it makes the app faster or more correct-feeling
to use; where a sub-item is architecture hygiene with no near-term perf/UX payoff, it's flagged as
such and deprioritised rather than done "because it's cleaner."

### Current reality (from reading `graphvertex.h` and `graph.h`'s private section in full)

`GraphVertex` (329 lines) mixes four unrelated concerns in one class:
- Real model data: `m_outEdges`/`m_inEdges` (adjacency), `m_number`.
- Visual presentation: size/color/shape/label styling, `m_x`/`m_y` position, `m_disp` (layout
  displacement).
- A flat result-cache: ~40 scalar fields, one raw+standardized pair per centrality/prestige index
  (`m_DC`/`m_SDC`, `m_BC`/`m_SBC`, `m_PRC`/`m_SPRC`, …), plus `m_distance`/`m_shortestPaths`
  (already targeted by the M1-continuation matrix migration).
- It inherits `QObject` — full vtable + signal/slot metadata overhead **per node** — to support
  exactly one signal, `signalSetEdgeVisibility`.

`Graph`'s private section (~190 lines) is the same pattern at graph scope: the actual vertex list
(`VList m_graph`) sits alongside IO/crawler infrastructure, 11 named `Matrix` objects (`SIGMA, DM,
sumM, invAM, AM, invM, WM, XM, XSM, XRM, CLQM` — WS5's territory), ~150 lines of
`mean*/variance*/min*/max*/group*` scalars mirroring `GraphVertex`'s per-index pattern, ~20 ad-hoc
`calculated*` boolean cache-validity flags with no shared invalidation mechanism, and
UI-interaction state with no business on a domain façade (`m_clickedEdge`, `m_vertexClicked`, even
`canvasWidth`/`canvasHeight`).

One good sign: `Graph`'s signal block already carries an explicit WS2-era comment — *"Signals are
a UI orchestration mechanism. Engines/services must not emit/call UI-facing behavior directly."*
— so the façade's signal-emitting role is intentional and doesn't need to move. M2 is about the
**data**, not the signals.

### Why each part earns its place (perf/UX lens)

- **`GraphVertex` dropping `QObject`** — direct, measurable performance win, not hygiene. Every
  node in every loaded network currently pays full `QObject` construction/memory overhead (vtable,
  signal/slot metadata, parent/child tracking) to support one narrow signal. For large networks —
  precisely the case WS3/WS10 care about — this is real, per-node cost paid on every load.
  Promoted from "worth a look" to a concrete M2 deliverable.
- **Explicit cache objects (feeds M4)** — the ~20 hand-managed `calculated*` flags are a *correctness/UX*
  risk as much as a performance one: a flag that isn't reset on the right mutation shows a **stale,
  wrong** centrality value to the user with no indication anything's off. A real cache abstraction
  with correct, dependency-aware invalidation fixes both the silent-staleness UX bug and avoids
  over-conservative invalidation that forces needless recomputation.
- **Matrix consolidation (feeds WS5 / M1-continuation)** — already directly performance-motivated:
  eliminating per-vertex QHash lookups in the back-propagation hot path in favour of flat array
  access.
- **`GraphModel` structural adapter itself** — the one item here that's primarily architectural, not
  a direct perf/UX win on its own. Justified as a prerequisite: it's what makes the two items above
  safe to do incrementally (regression-testable in isolation) instead of as one large, risky
  entangled change. Kept deliberately narrow for exactly this reason — see Approach below.

### Approach (adapter-first, per Work Rules)

1. Define `GraphModel` as a thin, QtCore-only class (no `QObject` inheritance) wrapping just the
   structural subset: vertex list, adjacency, relations. Starts as a **read-view adapter** over
   `Graph`'s existing `VList m_graph` — not a data migration yet, matching the "adapters first, not
   data migrations" Work Rule.
2. Explicitly **out of scope for M2**: centrality caches (→ M4), the 11 `Matrix` objects (→ WS5),
   the `calculated*` flags (→ M4 explicit cache objects). Keeping M2 to identity/structure only is
   what makes it a small, low-risk first cut instead of a rewrite.
3. Investigate `GraphVertex`'s `QObject`/`signalSetEdgeVisibility` removal as a concrete M2
   sub-task: check whether that one signal can be re-routed through `Graph`'s existing signal
   surface, then drop `GraphVertex` to a plain value class. Measure actual per-node memory
   difference on a large network before/after as the performance evidence for this specific change.
4. **Pre-existing commitment carried into M2:** both `roadmap_ws1_distances_geodesic_engine.md` (WS1,
   "What Remains Open") and `roadmap_ws2_ui_graph_facade.md` (WS2, "Optional Future Step") already
   named narrowing/removing the `friend class DistanceEngine;` access to `Graph` internals as
   explicitly deferred to "WS3 M2+" — found while auditing older roadmaps for stale/unfinished
   items (2026-07-25), and folded in here since `GraphModel`'s adapter boundary is exactly what
   would let `DistanceEngine` depend on a narrow interface instead of full `Graph` friendship.
5. **Completion criteria:**
   - `run_golden_compares.sh` and `run_benchmarks.sh` pass unchanged.
   - `GraphModel` is constructible and queryable with zero Qt widget/thread machinery — genuine
     headless testability, not just "compiles without QtWidgets".
   - At least one algorithm slice reads through `GraphModel` instead of `Graph` directly, as proof
     the adapter boundary actually holds before migrating anything else.
   - If the `QObject` removal lands: a measured per-node memory reduction on a large reference
     network (e.g. `geom.net`, 7343 nodes), not just "should be smaller" — matching this roadmap's
     own performance-evidence standard set by M1's benchmark tables.

## M3 — Move pure data containers out of UI/Qt dependencies

**Status: not yet scoped.** Depends on M2's shape — `GraphModel`'s adapter boundary determines
which containers can move and where they'd move to. Will be scoped once M2 lands, using the same
perf/UX lens as M2: each container move should be justified by a concrete win (memory, testability
that enables faster iteration on a real bug, etc.), not moved just to be "out of Graph."

## M4 — Gradually relocate caches into explicit cache objects

**Status: not yet scoped**, but its motivating evidence already exists from the M2 research pass:
`Graph`'s ~20 hand-managed `calculated*` boolean flags (no shared invalidation mechanism) are both
a performance risk (over-conservative invalidation forces needless recompute) and a UX/correctness
risk (a flag missed on the wrong mutation path shows a stale, wrong analysis result to the user
with no indication anything's off). Full scoping — which caches, what invalidation model — waits
for M2's `GraphModel` boundary to land first, but this is the concrete problem M4 exists to solve.

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

### #254 — UI responsiveness during long weighted-centrality computations ✅ Complete

**Problem:** computing a weighted, inverted-weight centrality index (e.g. Betweenness Centrality)
on a large network made the whole application completely unresponsive — not just slow, but unable
to repaint, receive input, or even respond to window-manager focus/switch requests — for the
entire duration of the computation. Confirmed identically present in v3.6 (not a regression):
for `geom.net` (7343 nodes, weighted, inverted, BC), the release build showed 795–900 % CPU across
~8 cores (`ps` state `R`, genuinely computing, not deadlocked) and eventually completed the
layout, but only after several minutes with the window completely frozen throughout. Also
confirmed present for FDP layouts (Eades/Fruchterman-Reingold/Kamada-Kawai) and every report/
analysis menu action that reaches `Graph::graphDistancesGeodesic()` — not just BC.

**Root cause:** `MainWindow` slots called `Graph` methods as direct, synchronous C++ calls from the
GUI thread. A direct method call always executes on the caller's thread regardless of the callee
`QObject`'s thread affinity — `Graph` being `moveToThread(&graphThread)`'d doesn't help unless the
call actually goes through a queued signal/slot connection, which none of these did. For indices
without a dedicated branch (including BC), this falls through to `DistanceEngine::compute()`
(`src/engine/distance_engine.cpp`), which uses `QtConcurrent::blockingMap(sources, ...)` to
parallelize per-source Dijkstra/BFS runs across the global thread pool — but `blockingMap` blocks
the *calling* thread until every worker finishes. The computation is genuinely parallelized, but
the calling thread never returns to its event loop for the whole duration.

**Fix:** `MainWindow::runGraphOperationAsync(operation, waitMessage, doneMessage)`
(`mainwindow.cpp`) dispatches `operation` onto `graphThread` via
`QMetaObject::invokeMethod(activeGraph, ..., Qt::QueuedConnection)` — not `QtConcurrent::run()`,
which was considered and rejected: `DistanceEngine::compute()` already parallelizes internally via
`blockingMap` over the *global* thread pool, so a second `QtConcurrent::run()` task would contend
with `blockingMap`'s own worker tasks for the same pool. Routing through `graphThread` (which
already exists — `Graph` lives there since `MainWindow::initGraph()`) sidesteps that entirely:
`blockingMap`'s behaviour is completely unaffected, only the identity of the thread that calls and
blocks on it changes. Shows an indeterminate `QProgressDialog` (no `setValue()` calls needed — cheap
regardless of network size, unlike the granular `showProgressBar` mechanism) which, being
`ApplicationModal`, also prevents the user mutating the graph from the GUI thread while `operation`
runs — required, since `DistanceEngine` and friends mutate `Graph`'s member state directly with no
internal synchronization.

Rolled out across all 28 entry points that reach `graphDistancesGeodesic()` (traced by grepping
every caller back to its enclosing public `Graph` method, then to whichever `MainWindow` slot calls
it directly): the 4 prominence-index layout slots, the 3 FDP layout slots, 11 report/analysis
methods with exactly one call site each (`writeEccentricity`, `writeCentralityCloseness`,
`writeCentralityBetweenness`, `writeCentralityStress`, `writeCentralityEccentricity`,
`writeCentralityPower`, `writeMatrixSimilarityMatching`, `writeMatrixSimilarityPearson`,
`vertexFindByIndexScore`, `graphDistanceGeodesic`, `graphDiameter`), 9 call sites into
`writeMatrix()` (the report dispatcher covering walks, similarity, and other matrix report types),
and 1 further entry point (`graphDistanceGeodesicAverage`/`isConnected`) found via a later re-audit
that the initial grep-based inventory missed.

**Two real bugs found and fixed during implementation, not just the happy path:**
- `busyDialog->close()` in the completion callback triggered `QProgressDialog`'s internal
  `cancel()` path, silently marking every *successful* completion as cancelled
  (`m_progressCanceled = true`), which made the next operation bail out immediately before doing
  any work. Fixed by using `reset()` instead — matching the pattern the existing granular progress
  dialog (`slotProgressBoxDestroy()`) already used, for exactly this reason.
- `graph_ui_prominence_distribution.cpp`'s three chart-building functions are plain `Graph::`
  methods that construct real QtCharts objects — safe only because every call used to run on the
  GUI thread by accident (see the addendum in `roadmap_ws2_ui_graph_facade.md`'s F4 section for the
  full story). Once these genuinely ran on `graphThread`, this became a live crash
  (`EXC_BAD_ACCESS` inside QtCharts). Fixed with `Graph::runOnGuiThread(std::function<void()>)`
  (`graph_ui_facade.cpp`) wrapping each function's entire body — fixed at the façade boundary, so
  it's safe for any caller, not just layouts. Swept all of `src/graph/` for the same pattern;
  confirmed this was the only instance.

Trailing "done" status messages (e.g. `"Nodes in inner circles have higher %1 score."`) moved from
running immediately after the (now non-blocking) call into the `doneMessage` parameter, fired from
`runGraphOperationAsync`'s completion callback — otherwise they'd display immediately, overwriting
the "please wait" message before the computation had actually finished.

**Secondary finding, still open:** `distance_engine.cpp` has ~75 unconditional `qDebug()` calls,
several inside the per-edge-relaxation inner loop (fires on the order of the total edge-relaxation
count — potentially billions for a graph this size). Unlike `qCDebug(category)`, plain `qDebug()`
isn't cheaply short-circuited by a disabled logging-category filter rule. Worth converting to a
dedicated category (same pattern as `lcGW` in `GraphicsWidget`) — not part of #254, filed separately
if it turns out to matter.

### GraphicsWidget — Performance and Code Quality Overhaul (#250) ✅ Complete

Split into its own file: [`roadmap_ws10_graphicswidget_overhaul.md`](roadmap_ws10_graphicswidget_overhaul.md).
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
