# ARCHITECTURAL_REFACTORING_ROADMAP

This document describes SocNetV’s architectural modernization plan at a **high level**.
Detailed, step-by-step work lives in separate mini-roadmaps under `docs/roadmaps/`.

---

# North Star

Move from a Graph-centric monolith:

```
UI → Graph → everything
```

to a layered design:

```
UI
↓
Graph (thin façade / coordinator)
↓
Domain model + services
   ├── algorithms
   ├── IO
   ├── matrices
   └── caches
```

Key outcomes:

* headless execution of algorithms
* deterministic regression baselines
* cleaner architectural boundaries
* easier maintenance and evolution
* incremental refactor without breaking behavior

---

# Guiding Principles

Non-negotiables:

* Preserve functionality and numeric results (bit-for-bit where possible).
* Preserve performance (no regressions vs stable releases).
* Keep refactors incremental: **compile → run → compare** after each step.
* UI behavior and progress signaling must remain stable during decoupling.

Preferred sequencing:

1. extract testable “engines” from monolithic Graph methods
2. build deterministic headless regression harnesses
3. stabilize boundaries
4. reorganize code into domain / services / IO layers

---

# Workstreams

---

# WS1 — Distances + Centralities Kernel Extraction (DONE)

Goal: make shortest-path and centrality computation testable, deterministic, and isolated.

Detailed plan:

```
docs/roadmaps/roadmap_distances_geodesic_engine.md
```

Status:

* `DistanceEngine` extracted to `src/engine/`
* Graph interaction narrowed through helper façade
* UI progress reporting moved to sink interface
* Headless CLI regression harness implemented
* Deterministic golden baselines introduced
* Micro-benchmarking guardrails implemented

Outcome:

Algorithms are now **engine-based and testable headlessly**.

---

# WS2 — Graph as Façade / Coordinator (DONE)

Goal: transform `Graph` from a monolithic algorithm host into a **thin coordinator and façade**.

Detailed plan:

```
docs/roadmaps/roadmap_ui_graph_facade.md
```

Major changes:

### Façade contract introduced

`Graph` now exposes explicit façade APIs for:

* UI
* CLI harness
* IO layer

### Mechanical decomposition of `graph.cpp`

The original ~19K LOC monolith was split into cohesive modules:

```
src/graph/
   core/
   storage/
   relations/
   filters/
   layouts/
   clustering/
   centrality/
   reporting/
   similarity/
   matrices/
   generators/
   reachability/
   ui/
```

### UI / Algorithm separation

Algorithm slices:

* compute data only
* must not construct Qt UI objects
* must not emit UI signals directly

UI responsibilities moved to:

```
src/graph/ui/
```

Example:

```
graph_prominence_distribution.cpp     (algorithm)
graph_ui_prominence_distribution.cpp  (QtCharts + PNG export)
```

### Progress façade introduced

Internal helpers inside `Graph`:

```
progressStatus()
progressCreate()
progressUpdate()
progressFinish()
```

These replace direct UI signal emissions inside algorithm slices.

Outcome:

`Graph` now acts as:

* **state holder**
* **invariants guardian**
* **algorithm coordinator**
* **UI signal dispatcher**

All changes verified via golden comparisons and performance benchmarks.

WS2 is structurally complete.

---

# WS4 — IO / Parser Refactor (ACTIVE)

Goal: move file loading toward a **clean IO layer** and remove tight coupling between `Parser` and `Graph`.

Detailed plan:

```
docs/roadmaps/roadmap_io_parser_refactor.md
```

Current roadmap document: 

---

## Historical Problem

Originally:

* `Parser` mutated `Graph` through **Qt signals**
* GUI and CLI wired signals independently
* mutation contracts were implicit
* deterministic headless testing was difficult

---

## Current State (Post-P4)

Parsing now uses an explicit mutation plane.

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

Mutation events include:

```
createNode(...)
createEdge(...)
setRelation(...)
addNewRelation(...)
removeDummyNode(...)
fileLoaded(...)
```

Legacy mutation signals have been **fully removed**.

GUI and CLI now share the **same mutation pipeline**.

---

## WS4 Milestones

### P1 — Parser API + mutation contract documented ✅ DONE

Parser mutation surface explicitly documented.

Completion condition defined:

```
Graph::signalGraphLoaded (preferred)
Parser::finished (fallback)
```

---

### P2 — Centralize Parser wiring helper ✅ DONE (later removed)

Originally introduced to prevent drift.
Later removed once sink architecture stabilized.

---

### P3 — Sink-based mutation architecture ✅ DONE

Key steps:

* Introduced `IGraphParseSink`
* Added `GraphParseSinkGraph`
* Parser forwards mutation events to sink
* GUI switched to sink-based loading
* Legacy Parser mutation signals removed

---

### P4 — Golden IO coverage + roundtrip regression harness ✅ DONE

Implemented via CLI regression tool.

Scripts:

```
scripts/run_golden_compares.sh
scripts/run_golden_io_roundtrip.sh
scripts/run_io_roundtrip_shipped_datasets.sh
```

Golden baselines stored in:

```
src/tools/baselines/io_roundtrip/
```

Coverage includes:

* GraphML
* Pajek
* Adj
* DOT
* DL
* GML
* EdgeList
* large benchmark datasets

Formats without exporters are baseline-locked as **export-skipped**.

---

## Current WS4 Focus

### P5 — Introduce `ParseConfig` boundary

Goal:

Reduce implicit coupling between Parser and Graph defaults.

Approach:

```
struct ParseConfig
{
   node defaults
   edge defaults
   canvas parameters
   parse flags
}
```

Parser will internally construct:

```
ParseConfig cfg
```

Parse handlers receive:

```
const ParseConfig&
```

Rules:

* no semantic changes
* no reordering of operations
* golden parity required

---

### P6 — Split parser.cpp by format

Current file:

```
src/parser.cpp (~200K LOC)
```

Target layout:

```
src/parser/
   parser_common.cpp
   parser_edgelist.cpp
   parser_adjacency.cpp
   parser_dl.cpp
   parser_pajek.cpp
   parser_graphml.cpp
   parser_gml.cpp
   parser_dot.cpp
```

Execution discipline:

* extract **one format per commit**
* build GUI + CLI
* run goldens
* run benchmarks

---

# WS3 — Domain Model Split (FUTURE)

Goal: introduce a domain model independent of UI concerns.

Detailed plan:

```
docs/roadmaps/roadmap_domain_model_split.md
```

Document reference: 

Target direction:

Separate:

```
model (nodes, edges, relations)
```

from:

```
algorithms
services
caches
```

Graph will remain a façade during transition.

Sequencing:

WS3 depends on:

* WS2 façade stabilization
* WS4 IO boundary cleanup

---

# WS5 — Matrices / Linear Algebra Modernization (SKELETON)

Goal:

Isolate matrix constructs and computations into a coherent module.

Plan:

```
docs/roadmaps/roadmap_matrices_modernization.md
```

Expected outcomes:

* dedicated matrix module
* cleaner linear algebra operations
* easier testing and reuse

---

# WS6 — Testing + CI + Regression Baselines (SKELETON)

Goal:

Prevent silent behavioral regressions.

Plan:

```
docs/roadmaps/roadmap_testing_ci_regression.md
```

Focus areas:

* deterministic baselines
* CI verification
* dataset coverage expansion

---

# WS7 — MainWindow Decomposition (SKELETON)

Goal: reduce the ~15K LOC `MainWindow` monolith.

Plan:

```
docs/roadmaps/roadmap_mainwindow_decomposition.md
```

Current responsibilities:

* menus
* toolbars
* dialogs
* canvas interaction
* settings persistence
* analysis result display

Target sub-controllers:

```
AppMenuController
AppToolbarController
StatusBarController
CanvasPanel
DialogManager
AppSettingsController
```

Rules:

* no UX changes
* purely structural refactor

---

# Workstream Dependency Graph

```
WS1 (Distance Engine)
   ↓
WS2 (Graph Façade)
   ↓
WS4 (IO / Parser Refactor)
   ↓
WS3 (Domain Model Split)
   ↓
WS5 (Matrices Modernization)

WS6 (Testing / CI)
   ↳ runs in parallel and supports all workstreams

WS7 (MainWindow Decomposition)
   depends on WS2
   benefits from WS4 progress
```

---

# Target Folder Layout (End State)

Conceptual architecture:

```
domain/
 ├── model/
 ├── algorithms/
 ├── matrices/
 ├── io/
 └── services/
```

This is a **long-term structural target**, not an immediate rewrite.

---

# Contribution Workflow

When contributing:

1. Choose a workstream in `docs/roadmaps/`
2. Follow its rules and sequencing
3. Keep commits **small and mechanical**
4. After each step:

```
build
./scripts/run_golden_compares.sh
./scripts/run_benchmarks.sh
```

Golden and performance baselines must remain stable.