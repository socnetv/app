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

graph_prominence_distribution.cpp
graph_ui_prominence_distribution.cpp

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

# WS4 — IO / Parser Refactor (DONE)

Goal: move file loading toward a **clean IO layer** and remove tight coupling between `Parser` and `Graph`.

Detailed plan:

```

docs/roadmaps/roadmap_io_parser_refactor.md

```

---

# Historical Problem

Originally:

* `Parser` mutated `Graph` through **Qt signals**
* GUI and CLI wired signals independently
* mutation contracts were implicit
* deterministic headless testing was difficult

---

# Current State (Post-WS4)

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

GUI and CLI now share the **same mutation pipeline**, ensuring deterministic mutation ordering during parsing.

---

# WS4 Milestones

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

### P5 — Introduce `ParseConfig` boundary ✅ DONE

Goal:

Reduce implicit coupling between Parser and Graph defaults.

Implementation:

```

struct ParseConfig
{
node defaults
edge defaults
canvas parameters
parse flags
}

```

Parser constructs:

```

ParseConfig cfg

```

Parse handlers receive:

```

const ParseConfig&

```

No behavioral or ordering changes were introduced.

---

### P6 — Split parser.cpp by format ✅ DONE

Original file:

```

src/parser.cpp (~5500 LOC)

```

Final layout:

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
parser.cpp

```

Extraction was performed:

* one format per commit
* golden regression verified
* benchmarks verified

Additional helper relocation occurred during extraction.

Examples:

* DL helpers → `parser_dl.cpp`
* Pajek helpers → `parser_pajek.cpp`
* Adjacency helpers → `parser_adjacency.cpp`
* shared helpers → `parser_common.cpp`

Result:

`parser.cpp` reduced to ~1200 LOC and now acts primarily as:

* load dispatcher
* parser lifecycle coordinator

---

# Post-WS4 Architectural State

Parser internals now look like:

```

Parser::load()
├── parser_edgelist.cpp
├── parser_adjacency.cpp
├── parser_dl.cpp
├── parser_dot.cpp
├── parser_gml.cpp
├── parser_pajek.cpp
└── parser_graphml.cpp

```

This provides:

* format-local parsing logic
* improved code locality
* smaller translation units
* safer future maintenance

All parser operations remain deterministic and protected by:

```

scripts/run_golden_compares.sh
scripts/run_benchmarks.sh

```

WS4 is now structurally complete.

---

# Post-WS4 Strategic Direction

With WS1, WS2, and WS4 completed, SocNetV now has:

- engine-based algorithms
- a façade-based Graph coordinator
- a deterministic IO mutation pipeline
- modular parser translation units
- regression harnesses for correctness and performance

This establishes a stable architectural base for the next development phase.

The immediate development priorities are:

1. **WS6 — Testing / CI / Regression expansion**  
2. **WS3 — Domain Model Split**  
3. **WS7 — MainWindow Decomposition**

These workstreams can proceed incrementally while maintaining deterministic regression guarantees.

---

## Immediate Focus — WS6

The next natural step is strengthening the **testing and regression infrastructure**.

Goals:

* expand deterministic regression coverage
* add more datasets and IO roundtrip baselines
* integrate automated CI verification
* prevent silent behavioral regressions during future refactors

The existing CLI regression harness already provides a strong foundation for this work.

---

## Mid-Term Focus — WS3

Once regression coverage is expanded, the next architectural milestone is the **Domain Model Split**.

This will introduce a domain model layer separating:

```

model (nodes, edges, relations)

```

from:

```

algorithms
services
caches

```

`Graph` will remain a façade coordinating these layers during the transition.

---

## Parallel / Later Work — WS7

`MainWindow` remains a large UI controller (~15K LOC).

Once core architecture stabilizes further, it can be decomposed into smaller UI controllers without affecting algorithm or IO layers.

This workstream is largely independent of the algorithm and IO architecture.


# WS3 — Domain Model Split (NEXT MAJOR STEP)

Goal: introduce a domain model independent of UI concerns.

Detailed plan:

```

docs/roadmaps/roadmap_domain_model_split.md

```

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

Both prerequisites are now complete.

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

# WS6 — Testing + CI + Regression Baselines (NEXT)

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

# WS8 — IO Layer Stabilization (FUTURE)

Goal:

Further refine the IO architecture introduced during WS4.

While WS4 modularized the parser implementation, future work may simplify
format dispatch and make the IO layer easier to extend.

Possible improvements:

* introduce a lightweight `FormatHandler` abstraction
* make the `Parser::load()` dispatch table more explicit
* allow format modules to self-register
* simplify adding new graph formats

Example conceptual direction:

```

Parser
↓
FormatHandler interface
├── GraphMLHandler
├── PajekHandler
├── GMLHandler
├── DotHandler
└── EdgeListHandler

```

Benefits:

* clearer IO architecture
* simpler format extensibility
* reduced complexity inside `Parser::load()`


# Workstream Dependency Graph

```

WS1 (Distance Engine)
↓
WS2 (Graph Façade)
↓
WS4 (IO / Parser Refactor)   ← completed
↓
WS6 (Testing / CI / Regression expansion)
↓
WS3 (Domain Model Split)
↓
WS5 (Matrices Modernization)

WS7 (MainWindow Decomposition)
depends on WS2
benefits from WS4 progress

WS8 (IO Layer Stabilization)
builds on WS4 parser modularization

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