# SocNetV Developer Notes

Technical reference for contributors. For the architectural direction and workstream roadmap, see:

* [`ARCHITECTURAL_REFACTORING_ROADMAP.md`](ARCHITECTURAL_REFACTORING_ROADMAP.md)

Detailed workstream plans:

```
docs/roadmaps/
```

---

# Architecture Overview

SocNetV is a Qt-based desktop application for social network analysis and visualization.

The architecture is layered across two threads:

```
┌─ main thread ──────────────────────────────────────────────────┐
│                                                                 │
│  MainWindow + dialogs          (menus, panels, status bar)      │
│       ↕ signals                                                 │
│  GraphicsWidget                (QGraphicsView canvas)           │
│    └─ GraphicsNode / GraphicsEdge  (QGraphicsItem scene items)  │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
         ↕ queued cross-thread signals (setNodePos, etc.)
┌─ graphThread ──────────────────────────────────────────────────┐
│                                                                 │
│  Graph                         (façade — state, invariants)     │
│    └─ Algorithm slices         (src/graph/<domain>/)            │
│    └─ UI façade layer          (src/graph/ui/ — charts, export) │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

`Graph` runs on `graphThread`; all canvas mutation goes through queued
signals to the main thread. Do **not bypass this flow** when adding new features.

---

# Graph as Façade

`Graph` is a state coordinator and invariant guardian — not a monolith. It holds graph state and delegates all computation to algorithm slices.

`graph.cpp` contains only:

```
Graph::Graph(...)
Graph::~Graph()
Graph::clear(...)
```

Everything else lives under `src/graph/`, organized by responsibility:

```
centrality/
clustering/
cohesion/
crawler/
distances/
filters/
generators/
io/
layouts/
matrices/
prominence/
reachability/
relations/
reporting/
similarity/
storage/
ui/
util/
```

---

# Structural Boundary Inside `src/graph/`

A strict separation is enforced between computation and rendering.

## Algorithm slices

- compute data only
- may use QtCore
- must **not** construct QtWidgets / QtCharts objects
- must **not** emit UI signals directly

Examples:

```
src/graph/prominence/graph_prominence_distribution.cpp
src/graph/centrality/graph_centrality.cpp
```

## UI façade layer (`src/graph/ui/`)

- constructs QtWidgets / QtCharts objects
- renders visualizations
- exports PNG charts
- emits UI update signals to `MainWindow`

Example:

```
src/graph/ui/graph_ui_prominence_distribution.cpp
```

## Rule for New Code

If you add new analytics:

1. **compute results in algorithm slices**
2. **render in the UI façade**

This separation is mandatory.

---

# Distance Engine

Shortest-path algorithms run through a dedicated engine:

```
src/engine/
  distance_engine.cpp
  distance_progress_sink.h
  graph_distance_progress_sink.cpp
  per_source_scratch.h        ← introduced in WS3 Phase 1
```

The engine runs from both the GUI and the CLI regression harness.

**WS3 parallelisation is complete (Phases 1 and 2 shipped).** The source loop in
`DistanceEngine::runAllSources()` now runs concurrently across all CPU cores via
`QtConcurrent::blockingMap`. Each worker thread owns a `ThreadLocalState` (see
`thread_local_state.h`) holding a reused `PerSourceScratch`, partial BC/SC accumulators,
and running totals for graph-wide aggregates. A single-threaded reduction step after
the loop merges everything into graph state.

Benchmark results (Debug build, 24-core Linux): 2.7×–8.3× speedup depending on network
size and whether centralities are computed. All 36 golden regression baselines pass.

Do not add new per-source mutable state to `Graph` or `GraphVertex` — put it in `PerSourceScratch`.

---

# Parsing and I/O

The parsing pipeline:

```
Parser
↓
IGraphParseSink
↓
Graph
```

Key components:

```
src/graph/io/graph_parse_sink.h
src/graph/io/graph_parse_sink_graph.cpp
```

Typical mutation calls:

```
createNode(...)
createEdge(...)
setRelation(...)
addNewRelation(...)
removeDummyNode(...)
fileLoaded(...)
```

GUI and CLI share the same mutation pipeline — deterministic behavior is guaranteed.

---

# Filter Layer

Non-destructive node/edge visibility filtering via snapshot/restore:

- `vertexFilterByEgoNetwork()`, `vertexFilterBySelection()`, `vertexFilterByAttribute()`
- `edgeFilterByWeight()`, `edgeFilterByAttribute()`, `vertexFilterByQuery()`
- `vertexFilterRestoreAll()` — replays the filter stack in reverse

All filters push a `FilterSpec` onto `m_visibilityHistory`. Undo restores the prior snapshot.

---

# Graph → Canvas Rendering System

## Threading model

`Graph` runs on a dedicated `graphThread`. All communication with
`GraphicsWidget` (canvas) is via **queued signal/slot connections** across
the thread boundary.

```
graphThread: Graph
     ↓  emit setNodePos(nodeNum, x, y)   [queued — cross-thread]
main thread: GraphicsWidget::moveNode()
     ↓  node->setPos(x, y)
     ↓  ItemPositionHasChanged → adjust() on all connected edges
```

`moveNode()` is intentionally one line. Do not add logic there.

## Layout signal discipline

Force-directed layouts (Eades, Fruchterman-Reingold, Kamada-Kawai) run all
iterations on `graphThread` and emit `setNodePos` **once per node at the
end**, not once per node per iteration. This avoids flooding the main
thread's event queue.

Static layouts (radial, levels, BC-based) emit exactly N signals in a
single pass — the minimum possible without a new bulk signal type.

**Do not add per-iteration `setNodePos` emissions** to any layout.

## Scene index method

`QGraphicsScene` defaults to `BspTreeIndex`, which rebuilds a spatial BSP
tree on every `prepareGeometryChange()` call. On large networks this is the
dominant performance cost: selecting or dragging nodes triggers O(E) edge
`adjust()` calls, each paying an O(log N) BSP update.

**The app defaults to `NoIndex`** (set via `appSettings["canvasIndexMethod"]`
at startup). This eliminates BSP overhead entirely. Hit-test cost becomes
O(N) per click, which is imperceptible for network analysis workloads.

The setting is user-controllable via **Settings > Canvas**. `BspTreeIndex`
remains available for small, static networks where fast hit-testing matters.

## Known hot path: node selection

`GraphicsNode::itemChange(ItemSelectedHasChanged)` calls `setSize()` and
`setColor()`, both of which call `prepareGeometryChange()` and trigger
`adjust()` on every connected edge. For N selected nodes this is O(N × avg
degree) geometry work, synchronous on the main thread. With `NoIndex` this
is fast in practice; with `BspTreeIndex` on large networks it causes
visible lag.

## Canvas performance knobs

All rendering flags are user-configurable via **Settings > Canvas**:

| Setting | Key | Default |
|---------|-----|---------|
| Scene index method | `canvasIndexMethod` | `NoIndex` |
| Viewport update mode | `canvasUpdateMode` | `Full` |
| Antialiasing | `antialiasing` | `true` |
| Cache background | `canvasCacheBackground` | `false` |
| Save painter state | `canvasPainterStateSave` | `false` |
| Edge highlighting | `canvasEdgeHighlighting` | `true` |

Do not hard-code scene rendering flags in `GraphicsWidget`'s constructor.
Apply them via the corresponding `slotOptionsCanvas*` methods, which read
from `appSettings` at startup.

---

# Regression Safety Harness

SocNetV ships a headless CLI tool for deterministic regression testing:

```
socnetv-cli
```

Documentation: [`docs/SOCNETV_CLI_REGRESSION_TOOL.md`](SOCNETV_CLI_REGRESSION_TOOL.md)

Scripts:

```
scripts/run_golden_compares.sh
scripts/run_benchmarks.sh
scripts/run_golden_io_roundtrip.sh
scripts/run_io_roundtrip_shipped_datasets.sh
```

These must pass after any structural change.

---

# CLI Kernel Architecture

The harness is organized around **kernel modules**. Each kernel covers a specific algorithm family and emits a versioned, deterministic JSON schema. Schemas are never modified after release.

Current kernels:

```
kernel_distance_v1      — geodesic distances + centralities
kernel_reachability_v2  — reachability matrix
kernel_walks_v3         — walks matrix A^K
kernel_prominence_v4    — all node-level centrality + prestige indices
kernel_io_roundtrip_v5  — load → export → reload signature comparison
kernel_clustering_v6    — clustering coefficient, triad census, clique census
kernel_connectivity_v7  — weakly connected components count + per-node IDs
```

Each kernel owns its execution logic, JSON schema, and comparison logic.

---

# Development Workflow

## Build

CMake + Qt6 (Linux / macOS / Windows). Keep changes incremental.

## Regression discipline

After any structural change:

```
./scripts/run_golden_compares.sh
./scripts/run_benchmarks.sh
```

Golden outputs and performance must remain stable.

## Current focus

**WS3 — DistanceEngine parallelisation** (Phases 1 and 2 complete). See
[`docs/roadmaps/roadmap_domain_model_split.md`](../roadmaps/roadmap_domain_model_split.md)
for the phased plan. Phase 3 (flat relation-keyed matrices) is delegated to WS5.

Bug fixes and issue triage continue alongside WS3. All changes are validated through
the WS6 regression harness.

---

# Launchpad PPA builds

```
Ubuntu 22.04 LTS (Jammy)
Ubuntu 24.04 LTS (Noble)
```
