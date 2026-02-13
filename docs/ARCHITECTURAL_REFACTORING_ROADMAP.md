# ARCHITECTURAL_REFACTORING_ROADMAP

This document describes SocNetV’s architectural modernization plan at a **high level**.
Detailed, step-by-step work lives in separate mini-roadmaps under `docs/roadmaps/`.

## North Star

Move from a Graph-centric monolith:

UI → Graph → everything

to a layered design:

UI
 ↓
Graph (thin façade / coordinator)
 ↓
Domain model + services (algorithms, IO, matrices, caches)

Key outcomes:
- headless execution of algorithms
- deterministic regression baselines
- cleaner boundaries and easier maintenance
- incremental refactor without breaking behavior


## Guiding Principles

Non-negotiables:
- Preserve functionality and numeric results (bit-for-bit where possible).
- Preserve performance (no regressions vs stable releases).
- Keep refactors incremental: compile + run + compare results at every step.
- UI behavior and progress signaling must remain stable while decoupling.

Preferred sequencing:
1) extract testable “engines” from monolithic Graph methods
2) build headless regression harnesses
3) only then reorganize code into domain/algorithms/services folders


## Workstreams

### WS1 — Distances + Centralities Kernel Extraction (DONE)
Goal: make the distance/shortest-path + centrality core testable and maintainable.

- Detailed plan: `docs/roadmaps/distances_geodesic_engine.md`

Status (summary):
- DistanceEngine extracted and parity verified
- UI progress decoupled via sink interface
- Headless CLI exists and prints deterministic metrics
- Golden output comparison + split engine out of graph.cpp safely
- Micro-benchmarking 
- Expand guardrail coverage (Phase E). This is ongoing/optional.


### WS2 — Graph as façade / coordinator (ACTIVE)
Goal: Graph becomes orchestration glue rather than algorithm host and state container.

- Plan: `docs/roadmaps/ui_graph_facade.md`


### WS3 — Domain model split (SKELETON)
Goal: establish a domain model that does not depend on Qt UI concerns.

- Plan: `docs/roadmaps/domain_model_split.md`


### WS4 — IO / Parser refactor (SKELETON)
Goal: move file loading toward a clean IO layer and reduce Qt signal entanglement.

- Plan: `docs/roadmaps/io_parser_refactor.md`


### WS5 — Matrices / linear algebra modernization (SKELETON)
Goal: isolate matrix constructs and computations into a coherent module.

- Plan: `docs/roadmaps/matrices_modernization.md`


### WS6 — Testing + CI + regression baselines (SKELETON)
Goal: make it hard to regress outputs silently.

- Plan: `docs/roadmaps/testing_ci_regression.md`


## Suggested Target Folder Layout (End State)

This is a conceptual target, not an immediate rewrite:

domain/
 ├── model/
 │    ├── GraphModel
 │    ├── Node
 │    ├── Edge
 │    ├── Relation
 │
 ├── algorithms/
 │    ├── distances/
 │    ├── centrality/
 │    ├── cohesion/
 │    ├── clustering/
 │    ├── similarity/
 │
 ├── matrices/
 │    ├── AdjacencyMatrix
 │    ├── LaplacianMatrix
 │
 ├── io/
 │    ├── GraphParser
 │
 └── services/
      ├── GraphAnalysisService
      ├── GraphMetricsCache


## How to Contribute

- Pick a workstream document in `docs/roadmaps/`
- Follow its “work rules”
- Keep commits small and verifiable (prefer one conceptual change per commit)
- Add/update regression outputs when changing anything algorithmic
