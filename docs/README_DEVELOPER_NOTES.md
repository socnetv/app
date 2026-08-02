# SocNetV Developer Notes

Technical reference for contributors. For the architectural direction and workstream roadmap, see:

* [`ARCHITECTURAL_REFACTORING_ROADMAP.md`](ARCHITECTURAL_REFACTORING_ROADMAP.md)

Detailed workstream plans:

```
docs/roadmaps/
```

> **Before committing any structural change:** run `./scripts/run_golden_compares.sh` (see
> "Regression discipline" under Development Workflow, below, for the full script list). All golden
> JSON baselines must still pass.

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
  thread_local_state.h
```

The engine runs from both the GUI and the CLI regression harness.

Call flow:

```
DistanceEngine::compute(computeCentralities, considerWeights, inverseWeights, dropIsolates)
  initRun()
  runAllSources()          ← parallel via QtConcurrent::blockingMap (WS3 Phase 2)
  finalize()
```

Scratch state is layered by scope — never held on `Graph`/`GraphVertex` directly:

| Struct | Scope | Location |
|---|---|---|
| `DistanceScratch` | Per run (N/E snapshots, iterators, connectivity) | `distance_engine.h` |
| `PerSourceScratch` | Per source vertex — reused within a thread | `src/engine/per_source_scratch.h` |
| `ThreadLocalState` | Per worker thread — owns a `PerSourceScratch` + partial BC/SC arrays | `src/engine/thread_local_state.h` |

**WS3 parallelisation is complete (Phases 1 and 2 shipped).** The source loop in
`DistanceEngine::runAllSources()` runs concurrently across all CPU cores via
`QtConcurrent::blockingMap`. Each worker thread owns a `ThreadLocalState` holding a reused
`PerSourceScratch`, partial BC/SC accumulators, and running totals for graph-wide aggregates. A
single-threaded reduction step after the loop merges everything into graph state.

Benchmark results (Debug build, 24-core Linux): 2.7×–8.3× speedup depending on network size and
whether centralities are computed.

**APSP results (geodesic distances, sigma counts) live in `Graph::m_apspDist`/`m_apspSigma`**
(`QHash<relation_id, Matrix>`, row = source vertex position, column = target vertex position;
read via `Graph::apspDistance()`/`apspShortestPaths()`) — not on `GraphVertex` (WS5 A2 migrated this
off a per-vertex `QHash` for lookup speed and memory; see
`roadmap_ws5_matrices_modernization.md`).

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

## `edgesHash` keying

`GraphicsWidget::edgesHash` (mapping a graph edge to its `GraphicsEdge` scene item — UI-layer
only, unrelated to `Graph`'s own data model) is keyed by `GraphicsWidget::edgeKey()`, a `quint64`
packed as `relation * 10^16 + v1 * 10^8 + v2`: a 3-digit relation field and two 8-digit node-number
fields, read left-to-right like the old `"relation:v1>v2"` string it replaced. This is positional
encoding, not a mixing hash — collisions are structurally impossible as long as each field stays
under its digit budget (relation < 1000, node numbers < 100,000,000 each), enforced by
`Q_ASSERT_X` in `edgeKey()`. Practical consequence: a single loaded network is capped at
99,999,999 nodes — not a real-world limit (parsers always assign sequential `1..N` node numbers
regardless of a source file's own IDs, and a `QGraphicsView` canvas with anywhere near that many
interactive items is unusable long before the cap matters).

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

See [`docs/ARCHITECTURAL_REFACTORING_ROADMAP.md`](../ARCHITECTURAL_REFACTORING_ROADMAP.md)
for active workstreams, priorities, and the long-term architecture direction.

## Continuous Integration (GitHub Actions)

`.github/workflows/build-ci.yml` (triggered by `[ci]`/`[gha]` in a commit message on `develop`)
and `build-release.yml` (triggered by pushing a tag) both drive Qt via
[`jurplel/install-qt-action`](https://github.com/jurplel/install-qt-action) (aqtinstall), which is
**independent of the target distro's own Qt6** — Launchpad PPA builds (see below) and the OBS RPM
build (`socnetv.spec`, unversioned `pkgconfig(Qt6*)` requirements) both build against whatever
Qt6 the distro's own package repos provide, not anything from these workflows. Changing the
`qt-version` matrix here only affects: the CI compile-compatibility smoke test, and the actual
macOS DMG / Windows installer / Linux AppImage artifacts that `build-release.yml` publishes for
tagged releases.

`build-ci.yml` tests both **qmake** and **cmake** per OS (two separate steps, each gated only on
`matrix.os`) — this used to be piggybacked on having two `qt-version` matrix entries (older
version → qmake, newer → cmake), which is why the `if:` conditions on those steps look qt-version
gated in git history; they no longer are, since the matrix currently carries a single version.

If bumping the pinned Qt version: check `https://download.qt.io/official_releases/qt/<minor>/`
for open-source availability first — Qt's newest LTS patch releases are sometimes commercial-only
for a window before reaching the open-source channel that `aqtinstall` draws from.

---

# Launchpad PPA builds

```
Ubuntu 22.04 LTS (Jammy)
Ubuntu 24.04 LTS (Noble)
```
