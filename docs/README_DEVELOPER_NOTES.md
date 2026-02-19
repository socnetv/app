# SocNetV Developer Notes

This folder documents the **current architecture** of SocNetV and the **ongoing modernization/refactor** effort.

If you are new to the codebase, start here, then read the high-level roadmap:
- [`ARCHITECTURAL_REFACTORING_ROADMAP.md`](ARCHITECTURAL_REFACTORING_ROADMAP.md)

For work-in-progress refactors (with detailed steps and status), see:
- `docs/roadmaps`


## Project Snapshot

SocNetV is a Qt-based desktop application for social network analysis and visualization.

Historically, most functionality flowed through a central `Graph` object (this is being incrementally decomposed under WS2) which acts as:
- domain model (network storage)
- algorithm host (distances, centralities, clustering, etc.)
- UI bridge (signals/progress)
- I/O coordinator (loading datasets)

This worked, but makes testing and incremental modernization hard.


## Code Shape

### Main high-level pieces

- UI layer (Qt Widgets):
  - `MainWindow`, dialogs, graphics widgets/items
- Core “god object”:
  - `Graph` (data + algorithms + UI signaling)
- Data structures:
  - `GraphVertex` / edges as storage and (importantly) **cache/state** for analysis results
- Algorithms:
  - Many analytics use `Graph::graphDistancesGeodesic()` (now backed by DistanceEngine) as a hub (distances + centralities)
- I/O:
  - `Parser` loads datasets and emits signals that `Graph` consumes

### Where work happens
The historical flow:
UI → Graph → (everything)

The target flow:
UI
 ↓
Graph (thin façade / coordinator)
 ↓
Domain model + services (algorithms, IO, matrices, caches)


## What’s Good (Pros)

- Mature functionality with years of real-world usage
- Efficient “one-pass” distance + centrality kernel (Brandes-style approach)
- Lots of debug output and UI progress signaling already in place
- GraphML and multiple formats supported


## Pain Points (Cons)

- `Graph` is a monolith: data, UI, algorithms, I/O, caches
- Algorithms are hard to test headlessly
- Stateful caches live inside `GraphVertex` and internal Graph containers
- Refactoring risk is high without a regression harness
- Threading / signals are tightly intertwined with analytics


## Current Refactor Strategy

We’re doing an [**incremental architectural refactor**](./ARCHITECTURAL_REFACTORING_ROADMAP.md) based on detailed roadmaps.

The active refactor currently underway is WS2 detailed at:
- [`./roadmaps/roadmap_ui_graph_facade.md`](./roadmaps/roadmap_ui_graph_facade.md)


### WS2 Status (Current)

WS2 (Graph as façade / coordinator) has completed the structural extraction phase (F2).

`graph.cpp` now contains only:
- `Graph::Graph(...)`
- `Graph::clear(...)`

All other responsibilities have been mechanically extracted into
dedicated translation units under `src/graph/`.

All changes preserve behavior and are verified via golden comparisons
and benchmark guardrails.

See:
- `ARCHITECTURAL_REFACTORING_ROADMAP.md`
- `docs/roadmaps/roadmap_ui_graph_facade.md`


### History

Previously, we implemented the refactor detailed in:
- `docs/roadmaps/distances_geodesic_engine.md`

This extracted the monolithic `Graph::graphDistancesGeodesic()` into a testable `DistanceEngine`
while preserving exact behavior. It also introduced a headless CLI that loads datasets and prints metrics.
Main results: 
- preserve behavior and numeric results
- keep changes small and verifiable
- add headless tooling and regression baselines early
- decouple UI progress signaling from algorithm logic

## Development Workflow Notes

### Build
- CMake + Qt 6 (macOS/Linux/Windows)
- Keep refactors incremental: compile/run/compare at every step.

### Debug output
- Debug output is treated as part of current behavior during refactor phases unless explicitly changed.

### Regression mindset
- Each refactor phase should produce measurable outputs (metrics and/or per-node vectors)
  and compare them against known good baselines.

### Where to look next
- High-level plan: `docs/ARCHITECTURAL_REFACTORING_ROADMAP.md`
- Detailed current work: `docs/roadmaps/roadmap_ui_graph_facade.md`
