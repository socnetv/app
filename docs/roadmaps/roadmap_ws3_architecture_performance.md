# Architecture & Performance Roadmap (WS3)

> **Before committing any change described in this roadmap:** run
> `./scripts/run_golden_compares.sh`. All golden JSON baselines must still pass — this applies to
> every phase, group, and item below, not just the ones that call it out explicitly.

## Goal

Fix specific, measured performance and correctness problems in `Graph`/`GraphVertex` — not build a
separate domain-model layer for its own sake.

**Note:** this roadmap originally aimed to introduce a separate domain model. That turned out
unnecessary — every real win here (M1's `PerSourceScratch` extraction, `GraphVertex`'s `QObject`
removal + signal batching) came from fixing a specific, measured problem directly, not from
restructuring on the assumption that separating concerns is inherently worth it. `GraphModel`, the
class built toward the original goal, was removed (`b9508c17`) after never finding a real use — see
the M2 section below for what happened and its commit messages for the detail. WS3 is now closed;
see [`ARCHITECTURAL_REFACTORING_ROADMAP.md`](../ARCHITECTURAL_REFACTORING_ROADMAP.md).

## Status at a Glance

| Milestone | Status |
|---|---|
| M1 — DistanceEngine parallelization | ✅ Done (v3.6), 2.7×–8.3× speedup |
| M1 continuation — flat relation-keyed matrices | 🔵 Delegated to WS5 |
| #254 / #263 — GUI freeze during long computations | ✅ Done |
| M2 — `GraphVertex` `QObject` removal + edge-visibility signal batching | ✅ Done |
| M3 — Move pure data containers out of UI/Qt dependencies | 🔴 Investigated, deferred — no justified candidate |
| M4 — Relocate caches into explicit cache objects | 🔴 Investigated, deferred — no justified candidate |
| GraphicsWidget canvas rendering | ✅ Phase 1 done, elevated to [WS10](roadmap_ws10_graphicswidget_overhaul.md) |

## Target Direction

- Fix a specific, measured problem in `Graph`/`GraphVertex` when one is found — don't restructure on
  the assumption that separating concerns is inherently worth it.
- Algorithm scratch state belongs in the algorithm, not in the domain objects — proven by M1, keep
  applying it when a similar concrete case (usually parallelization) appears.
- `Graph` stays the façade. There is no plan to introduce a separate domain-model layer.

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

## M2 — `GraphVertex` `QObject` removal + edge-visibility signal batching

**Status: ✅ Done (2026-07-29).** Originally scoped as "introduce `GraphModel`" — a read-view adapter
built first (`9c2461a0`) as a supposed prerequisite for the two real deliverables below. It wasn't
one (neither shipped commit depends on it — see "Removed" at the end of this section) and was deleted
2026-07-30 (`b9508c17`). What actually shipped and still stands: `GraphVertex`'s `QObject`
inheritance dropped, and the edge-visibility signal path batched (`4e07ec3f`). The rest of this
section is left as-written for the record — every reference to `GraphModel` below describes
something that existed for one day and no longer does.

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

**Correction (2026-07-29), found while scoping M3:** the last claim above doesn't hold up under a
direct code check, on both counts:
- `canvasWidth`/`canvasHeight` are read by algorithm slices (`graph_random_networks.cpp`,
  `graph_layouts_basic.cpp`, `graph_layouts_force.cpp` — layout generators need a bounding box to
  place nodes) and by the headless CLI path (`parser.cpp`, `tools/headless_graph_loader.cpp`).
  They're legitimate layout parameters, not UI leftovers — moving them out would break headless
  generation/parsing.
- `m_clickedEdge`/`m_vertexClicked` (`graph_selection.cpp`) back a real computation — resolving a
  click into a fully-typed edge/vertex description via `m_graph`/`vpos` lookups, `edgeExists()`,
  `isDirected()` — not passive click bookkeeping. It's already correctly placed: `graph_selection.cpp`
  lives in `src/graph/ui/`, this codebase's own UI-façade layer, whose documented job is exactly
  "constructs widgets, renders charts, emits UI update signals; corresponding compute lives in the
  sibling algorithm slice." Moving these fields to `MainWindow` would force `MainWindow` to reach
  past `Graph` into `m_graph`/`vpos` directly — a layered-call-flow bypass this codebase's
  architecture explicitly forbids. Usage is confined to `graph.cpp` (reset) and `graph_selection.cpp`
  only — an earlier pass over this also mis-flagged `mainwindow.h`'s `m_clickedEdgeToggleBtn`/
  `m_clickedEdgeSection` as hits; those are unrelated `QPushButton`/`QWidget` members that merely
  share a name prefix, not the same fields.

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
- **`GraphModel` structural adapter itself** — the one item here that was primarily architectural, not
  a direct perf/UX win on its own. Justified at the time as a prerequisite: supposedly what makes the
  two items above safe to do incrementally. **This did not hold up** — the commit that actually shipped
  the `QObject` removal and batching (`4e07ec3f`) never touches `GraphModel`, so the "prerequisite"
  claim was never exercised. `GraphModel` was removed 2026-07-30 (see the Goal section's note).

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
4. **Correction (2026-07-29) to a claim carried in from WS1/WS2:** both
   `roadmap_ws1_distances_geodesic_engine.md` (WS1, "What Remains Open") and
   `roadmap_ws2_ui_graph_facade.md` (WS2, "Optional Future Step") named narrowing/removing the
   `friend class DistanceEngine;` grant as deferred to "WS3 M2+," on the assumption that
   `GraphModel`'s structural adapter boundary is what `DistanceEngine` needs it for. **Checked
   directly against the code before implementing M2** (grepped every one of the ~85 distinct
   `graph.X` members `DistanceEngine` actually touches via the friend grant): every single one is
   either an already-public accessor (`verticesBegin()`, `vertices()`, `vertexAtIndex()`, …) or a
   private *cache/aggregate* member (`maxSCC`, `discreteCCs`, `calculatedCentralities`, the
   `minmax()`/`resolveClasses()` helpers) — squarely **M4 territory** (explicit cache objects), not
   the structural vertex-list/adjacency data M2 covers. A structural-only `GraphModel` does not, by
   itself, let `DistanceEngine` drop the friend declaration — that depends on M4 landing instead.
   Recorded here so this doesn't get assumed again.
5. **Completion criteria:**
   - `run_golden_compares.sh` and `run_benchmarks.sh` pass unchanged.
   - `GraphModel` is constructible and queryable with zero Qt widget/thread machinery — genuine
     headless testability, not just "compiles without QtWidgets".
   - At least one algorithm slice reads through `GraphModel` instead of `Graph` directly, as proof
     the adapter boundary actually holds before migrating anything else.
   - If the `QObject` removal lands: a measured per-node memory reduction on a large reference
     network (e.g. `geom.net`, 7343 nodes), not just "should be smaller" — matching this roadmap's
     own performance-evidence standard set by M1's benchmark tables.

### Shipped (2026-07-29)

`GraphModel` (`src/graph/core/graph_model.h`/`.cpp`) is a plain, non-`QObject` class holding a
`Graph&` and exposing `vertexCount()`, `degreeOut(int)`, `degreeIn(int)` — built entirely on
`Graph`'s existing public accessors (`vertexIndexByNumber()`, `vertexAtIndex()`, `vertices()`), so
it required **no new friend access**. `vertexAtIndex(vertexIndexByNumber(v))` is exactly equivalent
to the `m_graph[vpos[v]]` pattern used internally throughout `Graph` — same O(1)/O(logN) lookup
cost, confirmed by reading both implementations, plus one small correctness improvement: it returns
a safe fallback for an unknown vertex number instead of `vpos`'s silent phantom-entry behavior on a
missing key.

First real caller migrated as proof the adapter boundary holds: `Graph::vertexDegreeOut()` and
`Graph::vertexDegreeIn()` (`graph_structure_metrics.cpp`) now call `GraphModel(*this).degreeOut()`/
`degreeIn()` instead of touching `m_graph`/`vpos` directly. `run_golden_compares.sh` and
`run_golden_io_roundtrip.sh` pass unchanged (correctness, not timing-sensitive — meaningful
evidence). `run_benchmarks.sh` also passed, but **that result isn't meaningful evidence of no
regression for this specific change** — the committed baseline (`scripts/perf_baselines/macos-arm64/
perf_expected.env`) is from 2026-03-03, five months and 226 `src/`-touching commits stale, so
"beats baseline by 30–70%" reflects that much unrelated drift, not anything about this migration.
The actual evidence for no performance regression here is architectural: `vertexAtIndex(vertexIndexByNumber(v))`
is exactly equivalent in complexity to the `m_graph[vpos[v]]` pattern it replaces (confirmed by
reading both implementations), so there's no reason to expect a difference. Re-recording the perf
baseline is tracked separately — not blocking this change, but worth doing so future timing
comparisons here are actually trustworthy.

**Second commit (`4e07ec3f`), same day:** `GraphVertex` dropped `QObject`/`Q_OBJECT`/its
`signals:` block. Its one signal (`signalSetEdgeVisibility`, relayed 1:1 into `Graph`'s
identically-shaped signal via a same-thread `Direct` connection) was its only `QObject`-dependent
behaviour — confirmed by a full-codebase grep for `deleteLater`, `qobject_cast`, `QPointer`,
`moveToThread`, `tr()`, or any other `connect()` involving `GraphVertex`; none exist. It now calls
plain methods on its owning `Graph` (`notifyEdgeVisibilityChanged()` /
`notifyEdgesVisibilityBatch()`) instead of emitting its own signal.

Measured `sizeof(GraphVertex)`: 584 → 568 bytes (16 bytes/vertex from the removed vtable pointer +
inline `QObject` state), ×7343 nodes on the `geom.net` reference network ≈ 117 KB — plus eliminating
`QObjectPrivate`'s separate heap allocation entirely, per vertex, which the `sizeof()` diff alone
doesn't capture. This is a struct-layout measurement extrapolated across node count, not a live
RSS/heap-profiler reading on a loaded process — real evidence, but worth being explicit that it's
extrapolated, not measured live.

Same commit also batched the two bulk edge-visibility call sites: `Graph::relationSet()` and
`Graph::edgeFilterUnilateral()` previously emitted `signalSetEdgeVisibility` once per edge — each a
separate queued cross-thread `QMetaCallEvent` (`graphThread` → GUI thread) — potentially thousands
of individual dispatches for one relation switch or unilateral-edge toggle on a large network. Both
now collect a `QList<EdgeVisibilityChange>` and fire one `signalSetEdgesVisibilityBatch` at the end;
`GraphicsWidget::setEdgesVisibilityBatch()` applies the whole batch through the existing, unchanged
`setEdgeVisibility()` in a plain loop — per-edge canvas behaviour (hash lookups, reverse-edge
handling, possible edge creation/deletion) is byte-for-byte identical to before; only the dispatch
count changed.

**Correctness evidence for the batching change specifically:** seven new `--interactive-script`
commands (`relation`, `unilateral`, `erdos`, `save`, `add-node`, `add-edge`, `add-relation` — #262,
`3e9d0245`, see [`roadmap_ws12_cli_scripting_mode.md`](roadmap_ws12_cli_scripting_mode.md)) were
built specifically to drive both batched call sites end-to-end through the real Qt event loop on
real networks (a small deterministic ER graph, then Krackhardt), covering relation switches,
unilateral toggles, and multi-relation graphs with newly-added nodes/edges. No bugs found. Combined
with a direct parameter-order audit against the original per-edge emit calls and a log-grep
confirming no "cannot queue arguments" metatype warnings for `EdgeVisibilityChange` crossing
threads, this is the actual evidence base for "no regression" here — `run_benchmarks.sh` passing is
not, for the same stale-baseline reason noted above (baselines have since been re-recorded against
`v3.6`, see `docs/roadmaps/roadmap_ws6_testing_ci_regression.md` and the perf-baseline commits).

**Completion criteria — checked off:**
- ✅ `run_golden_compares.sh` / `run_golden_io_roundtrip.sh` pass unchanged (re-verified against
  current `develop` HEAD, both commits included).
- ✅ `GraphModel` is constructible and queryable with zero Qt widget/thread machinery.
- ✅ `Graph::vertexDegreeOut()`/`vertexDegreeIn()` read through `GraphModel` instead of `Graph`
  directly.
- ✅ `QObject` removal shipped, with a measured (if extrapolated, see above) per-node memory
  reduction on the `geom.net`-scale reference network.

### Removed (2026-07-30)

`GraphModel` was deleted (`b9508c17`): its stated prerequisite role was never exercised (the commit
above shipped without it), its headless-testability rationale was already satisfied by `Graph` itself
via `socnetv-cli`, and it had one trivial caller and zero references from any other workstream. Both
callers reverted to the pre-`GraphModel` `m_graph[vpos[v1]]->...()` form. What remains permanently
shipped from M2 is only the `QObject` removal and signal batching described above.

## M3 — Move pure data containers out of UI/Qt dependencies

**Status: investigated, deferred — no candidate clears the perf/UX bar.** The two leads from the M2
audit were both wrong (`canvasWidth`/`canvasHeight` are genuine headless layout parameters;
`m_clickedEdge`/`m_vertexClicked` are already correctly placed in the UI-façade layer). The one real
candidate — giving a structural adapter real ownership of `VList m_graph`/`vpos` instead of just
reading through `Graph` — was checked and rejected: `vpos` is touched at ~140 call sites across 18
files, and `Graph::vertexRemove()` does a fragile hand-rolled `O(N)` reindex of it on every deletion
(already carrying its own defensive consistency check, a sign this has broken before). Ownership
transfer buys zero performance — same lookup, same complexity, wherever the hash lives — only a
speculative future testability benefit with no currently-blocked test behind it. **Deferred
indefinitely**; pick back up only if a concrete need appears.

## M4 — Gradually relocate caches into explicit cache objects

**Status: investigated, deferred — stated motivation checked and found false.** M2's audit claimed
`Graph`'s ~20 `calculated*` cache flags had "no shared invalidation mechanism." Checked directly
against `Graph::setModStatus()` (`graph_metadata.cpp:159-193`): a shared mechanism already exists,
called from 66 sites, and correctly invalidates at every real structural mutator checked
(`vertexCreate`/`Remove`, `edgeCreate`/`Remove`, `relationSet`, `edgeFilterUnilateral`). No live
staleness bug found. What's left is coarse-grained invalidation (safe, but wipes all caches on any
structural change) with no measured recompute cost, and a real but not-currently-felt maintenance tax
(new analyses require hand-editing the same shared reset block). **Deferred** alongside M3, for the
same reason: no concrete, currently-felt problem to fix.

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

**Secondary finding — ✅ confirmed and spun out into [WS14](roadmap_ws14_logging_cost.md).**
`distance_engine.cpp` has ~75 unconditional `qDebug()` calls, several inside the per-edge-relaxation
inner loop (fires on the order of the total edge-relaxation count — potentially billions for a graph
this size). Unlike `qCDebug(category)`, plain `qDebug()` isn't cheaply short-circuited by a disabled
logging-category filter rule. Worth converting to a dedicated category (same pattern as `lcGW` in
`GraphicsWidget`) — not part of #254, filed separately "if it turns out to matter."

**It turned out to matter, by a lot.** Measured 2026-07-30 by comparing two Release builds differing
only in `-DQT_NO_DEBUG_OUTPUT`: removing this formatting cost makes `DistanceEngine` **43×–72×
faster** (e.g. 2000N/40000E distances 51672 ms → 943 ms; ER N1000/E19879 with centralities
17186 ms → 237 ms), with all golden baselines passing unchanged. That is roughly an order of
magnitude more than M1's entire parallelisation win recorded above — and the measurement is
*conservative*, since `socnetv-cli` already discards debug output, so it reflects pure string
formatting that is then thrown away. The mechanism recorded above was exactly right; only the
magnitude was unknown. Full evidence, call-site inventory, and milestones:
[`roadmap_ws14_logging_cost.md`](roadmap_ws14_logging_cost.md).

### #263 — Information/Eigenvector Centrality and Walks Total still block the GUI ✅ Complete

Direct continuation of #254, same fix pattern: `MainWindow::runGraphOperationAsync()` dispatches an
operation onto `graphThread` via `QMetaObject::invokeMethod(..., Qt::QueuedConnection)`, already
proven safe and mechanical across 28 entry points — extended to 3 more.

These three were correctly out of #254's original scope — confirmed via grep that none of them call
`Graph::graphDistancesGeodesic()`, so they never went through the code path #254 fixed:

- **Information Centrality** (`slotAnalyzeCentralityInformation` → `Graph::writeCentralityInformation`)
  — matrix inversion via LU decomposition.
- **Eigenvector Centrality** (`slotAnalyzeCentralityEigenvector` → `Graph::writeCentralityEigenvector`)
  — power iteration on the adjacency matrix.
- **Walks Total** (`slotAnalyzeWalksTotal` → `Graph::writeMatrixWalks`) — sociomatrix powers up to N-1.

Pre-flight checks (node-count guard, the SLOW/VERY SLOW warning dialogs, `askAboutEdgeWeights()`)
stay synchronous on the GUI thread; only the computation + file write moved onto `graphThread`.
`writeMatrixWalks()` returns `void`, so its completion is checked via `activeGraph->progressCanceled()`
in the completion callback instead of the `shared_ptr<bool> success` pattern the other two use.

**Separate finding, fixed separately as #266:** Walks Total's underlying computation
(`Graph::graphWalksMatrixCreate()`, `graph_reachability_walks.cpp`) sums the adjacency matrix raised
to *every* power from 1 to N-1, stored as `qreal`/`double`. For networks past roughly a few dozen
nodes, walk counts at high powers vastly exceed `double`'s ~15-17 significant digits — the existing
warning dialog only warns about *speed*, not numerical validity, so the feature could silently
produce hundred-plus-digit cell values in the HTML report, printed in full via `Qt::fixed`, looking
exact when almost none of those digits were meaningful. Reported live on a 250-node Erdős–Rényi
network. **The summation formula itself is correct** — total walks of any length ≤ N-1 is the
standard SNA definition — this was a display-honesty issue, not a computation bug. Fixed by
switching `writeMatrixHTMLTable()` (`graph_reports.cpp`, shared by every matrix report) to
scientific notation for any cell magnitude beyond 1000, rather than attempting arbitrary-precision
arithmetic — a 300-digit "exact" walk count wouldn't be a more useful statistic than an honestly
imprecise one.

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
