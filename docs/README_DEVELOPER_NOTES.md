# SocNetV Developer Notes

This folder documents the **current architecture** of SocNetV and the **ongoing modernization effort**.

If you are new to the codebase, start here, then read the high-level refactoring roadmap:

* [`ARCHITECTURAL_REFACTORING_ROADMAP.md`](ARCHITECTURAL_REFACTORING_ROADMAP.md)

Then read the current product-oriented roadmap:

* [`docs/roadmaps/roadmap_graph_exploration.md`](docs/roadmaps/roadmap_graph_exploration.md)

Detailed execution plans live under:

```

docs/roadmaps/

```

---

# Project Snapshot

SocNetV is a Qt-based desktop application for social network analysis and visualization.

Historically, most functionality flowed through a central `Graph` object which acted as:

* domain model (network storage)
* algorithm host (distances, centralities, clustering, etc.)
* UI bridge (signals/progress)
* I/O coordinator (loading datasets)

This design worked but made testing, modularization, and safe refactoring difficult.

---

# Regression Safety Harness

To safeguard the modernization effort, SocNetV includes a **headless regression harness**:

```

socnetv-cli

```

This tool allows deterministic execution of algorithms and parsing pipelines.

It supports:

* golden output comparisons
* performance benchmarking
* IO roundtrip validation

Documentation:

```

src/tools/SOCNETV_CLI_REGRESSION_TOOL.md

```

Scripts:

```

scripts/run_golden_compares.sh
scripts/run_benchmarks.sh
scripts/run_golden_io_roundtrip.sh
scripts/run_io_roundtrip_shipped_datasets.sh

```

These scripts must pass after structural refactors.

The CLI tool executes deterministic algorithm kernels and compares results
against committed JSON baselines. This guarantees that architectural
refactors do not change algorithm outputs or graph semantics.

---

# CLI Kernel Architecture

The regression harness is organized around **kernel modules**.

Each kernel protects a specific algorithm family and emits a deterministic JSON schema.

Current kernels include:

```

kernel_distance_v1
kernel_reachability_v2
kernel_walks_v3
kernel_prominence_v4
kernel_io_roundtrip_v5

```

Each kernel owns:

* its execution logic
* JSON schema definition
* strict comparison logic

Schemas are versioned and never modified after release.

This ensures deterministic verification of algorithm correctness during
architectural refactors.

---

# Current Architectural State

Refactoring workstreams **WS1, WS2, and WS4 are complete**.

The project now has:

* engine-based algorithms
* a thin Graph façade
* a deterministic regression harness
* unified GUI/CLI parsing via `IGraphParseSink`

---

# Current Development Focus

## Primary Workstream

**WS9 — Graph Exploration & Data Workflows**

Defined in:

```

docs/roadmaps/roadmap_graph_exploration.md

```

Focus areas:

* graph filtering (structural + attribute-based)
* subgraph exploration workflows
* structured data editing (nodes/edges as tables)
* CSV / JSON import-export
* future query system and temporal filtering

---

## Supporting Workstream

**WS6 — Testing / CI / Regression**

All development must be validated through:

* CLI regression harness
* golden comparisons
* benchmarks

WS6 ensures:

* safe refactoring
* deterministic behavior
* performance stability

---

# Graph as Façade

`Graph` now acts primarily as:

* state holder and invariants guardian
* explicit façade API for UI and CLI
* delegator to algorithm slices
* central UI signal coordinator

`graph.cpp` contains only:

```

Graph::Graph(...)
Graph::clear(...)

```

All other functionality lives under:

```

src/graph/

```

organized by responsibility:

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

A strict separation is enforced.

## Algorithm slices

Responsibilities:

* compute data only
* may use QtCore
* must **not** construct QtWidgets / QtCharts objects
* must **not** emit UI signals directly

Examples:

```

src/graph/prominence/graph_prominence_distribution.cpp
src/graph/centrality/graph_centrality.cpp

```

---

## UI façade layer (`src/graph/ui/`)

Responsibilities:

* construct QtWidgets / QtCharts objects
* render visualizations
* export PNG charts
* emit UI update signals to `MainWindow`

Example:

```

src/graph/ui/graph_ui_prominence_distribution.cpp

```

---

## Rule for New Code

If you add new analytics:

1. **compute results in algorithm slices**
2. **perform rendering in the UI façade**

This separation is mandatory.

---

# Distance Engine

Shortest-path algorithms were extracted into:

```

src/engine/

```

Main components:

```

distance_engine.cpp
distance_progress_sink.h
graph_distance_progress_sink.cpp

```

These engines can run:

* from the GUI
* from the CLI regression harness

---

# Parsing and I/O

The parser architecture was modernized during **WS4**.

---

## Current Parsing Architecture

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

GUI and CLI share the same mutation pipeline → deterministic behavior.

---

# Code Shape (High-Level)

## UI Layer

```

MainWindow
dialogs
graphics widgets/items

```

---

## Core Coordinator

```

Graph (façade)

```

---

## Data Structures

```

GraphVertex
edge storage
analysis caches

```

---

## Engines / Algorithm Slices

Examples:

```

DistanceEngine
centrality
clustering
prominence
similarity
layouts
generators
reporting
reachability

```

---

## IO

```

Parser
IGraphParseSink
GraphParseSinkGraph

```

---

# Development Workflow Notes

## Build

* CMake + Qt6 (Linux / macOS / Windows)
* Refactors must remain incremental

---

## Regression Discipline

After structural changes:

```

./scripts/run_golden_compares.sh
./scripts/run_benchmarks.sh

```

Golden outputs and performance must remain stable.

---

# Launchpad PPA builds

Supported:

```

Ubuntu 22.04 LTS (Jammy)
Ubuntu 24.04 LTS (Noble)

```

---

# Mental Model for Contributors

```

UI
↓
Graph (façade)
↓
Algorithm slice / engine
↓
UI façade (if rendering required)
↓
Signal to MainWindow

```

Do **not bypass this flow**.

---

# Development Philosophy (Important)

SocNetV evolves through:

### 1. Product-driven development (WS9)

- deliver real user value
- enable large-network exploration
- build on existing systems

### 2. Incremental architectural evolution

- refactor safely using WS6 harness
- avoid large disruptive rewrites
- let real usage guide abstraction (WS3, WS7)
