# SocNetV Developer Notes

This folder documents the **current architecture** of SocNetV and the **ongoing modernization effort**.

If you are new to the codebase, start here, then read the high-level refactoring roadmap:

* [`ARCHITECTURAL_REFACTORING_ROADMAP.md`](ARCHITECTURAL_REFACTORING_ROADMAP.md)

Detailed execution plans live under:

* `docs/roadmaps/`

---

## Project Snapshot

SocNetV is a Qt-based desktop application for social network analysis and visualization.

Historically, most functionality flowed through a central `Graph` object which acted as:

* domain model (network storage)
* algorithm host (distances, centralities, clustering, etc.)
* UI bridge (signals/progress)
* I/O coordinator (loading datasets)

This design worked but made testing, modularization, and safe refactoring difficult.

To safeguard our modernization effort, we have a harness tool, called `socnetv-cli`.
The tool supports golden compares and benchmarking. 
See [SOCNETV_CLI_REGRESSION_TOOL.md](../src/tools/SOCNETV_CLI_REGRESSION_TOOL.md)
Also, there are automated scripts to run:
-  [golden compares](../scripts/run_golden_compares.sh) 
-  [benchmarks](../scripts/run_benchmarks.sh).

---

## Current Architectural State

Refactoring workstreams WS1 and WS2 are complete.

### Graph as Façade

`Graph` now acts primarily as:

* state holder and invariants guardian
* explicit façade API for UI and CLI
* delegator to algorithm slices
* central UI signal coordinator

`graph.cpp` contains only:

* `Graph::Graph(...)`
* `Graph::clear(...)`

All other functionality lives under:

```
src/graph/
```

organized by responsibility (distances, centrality, clustering, prominence, layouts, io, ui, etc.).

---

## Structural Boundary Inside `src/graph/`

A strict separation is now enforced:

### Algorithm slices

* Compute data only
* May use QtCore
* Do **not** construct QtWidgets / QtCharts objects
* Do **not** emit UI signals directly

### UI façade layer (`src/graph/ui/`)

* Constructs QtCharts / QtWidgets objects
* Handles PNG exports
* Emits signals to `MainWindow`

If you add new analytics:

* Keep computation in algorithm slices
* Delegate rendering and signal emission to the UI façade

This rule is mandatory for new code.

---

## Code Shape (High-Level)

### UI Layer

* `MainWindow`
* dialogs
* graphics widgets/items

### Core Coordinator

* `Graph` (façade)

### Data Structures

* `GraphVertex`
* edge storage
* analysis caches

### Engines / Slices

* DistanceEngine
* centrality
* clustering
* prominence
* similarity
* layouts
* generators
* reporting
* io wrappers

### I/O

* `Parser` (next major refactor target — WS4)

---

## Current Refactor Direction

High-level plan:

* `ARCHITECTURAL_REFACTORING_ROADMAP.md`

WS2 (Graph façade) is complete.

Next focus:

* WS4 — IO / Parser boundary cleanup

The goal is to clarify loading flow and reduce Qt signal entanglement before attempting a domain model split (WS3).

---

## Development Workflow Notes

### Build

* CMake + Qt 6 (macOS/Linux/Windows)
* Refactors must be incremental and verifiable.

### Regression Discipline

* Golden comparisons must pass. See scripts/run_golden_compares.sh
* Performance must remain within benchmark guardrails. See scripts/run_benchmark.sh
* Behavior preservation is mandatory.

### Debug Output

* Considered part of observable behavior during refactors unless explicitly changed.


### Launchpad PPA builds

Launchpad PPA builds are officially supported for:
- Ubuntu 22.04 LTS (Jammy)
- Ubuntu 24.04 LTS (Noble)

Other Ubuntu series are intentionally disabled to reduce maintenance surface and dependency drift.

---

## Mental Model for Contributors

Current runtime flow:

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

Do not bypass this flow.

If you are unsure whether something belongs in an algorithm slice or in the UI façade layer, prefer separation.

