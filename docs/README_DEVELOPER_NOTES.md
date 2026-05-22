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

The architecture is layered:

```
UI (MainWindow + dialogs + graphics)
↓
Graph (thin façade / coordinator)
↓
Algorithm slices / engines
↓
UI façade layer (rendering, chart export)
↓
Signal to MainWindow
```

Do **not bypass this flow** when adding new features.

---

# Graph as Façade

`Graph` is a state coordinator and invariant guardian — not a monolith. It holds graph state and delegates all computation to algorithm slices.

`graph.cpp` contains only:

```
Graph::Graph(...)
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
```

The engine runs from both the GUI and the CLI regression harness.

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
- `vertexFilterRestore()` — replays the filter stack in reverse

All filters push a `FilterSpec` onto `m_visibilityHistory`. Undo restores the prior snapshot.

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

Bug fixes and issue triage. See the open issues on GitHub.

All changes are validated through the WS6 regression harness.

---

# Launchpad PPA builds

```
Ubuntu 22.04 LTS (Jammy)
Ubuntu 24.04 LTS (Noble)
```
