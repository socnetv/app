# ARCHITECTURAL_REFACTORING_ROADMAP

This document describes SocNetV’s architectural modernization plan at a **high level**.
Detailed, step-by-step work lives in separate mini-roadmaps under `docs/roadmaps/`.

---

## North Star

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
Domain model + services (algorithms, IO, matrices, caches)
```

Key outcomes:

* headless execution of algorithms
* deterministic regression baselines
* cleaner boundaries and easier maintenance
* incremental refactor without breaking behavior

---

## Guiding Principles

Non-negotiables:

* Preserve functionality and numeric results (bit-for-bit where possible).
* Preserve performance (no regressions vs stable releases).
* Keep refactors incremental: compile + run + compare results at every step.
* UI behavior and progress signaling must remain stable while decoupling.

Preferred sequencing:

1. extract testable “engines” from monolithic Graph methods
2. build headless regression harnesses
3. only then reorganize code into domain/algorithms/services folders

---

## Workstreams

---

### WS1 — Distances + Centralities Kernel Extraction (DONE)

Goal: make the distance/shortest-path + centrality core testable and maintainable.

* Detailed plan: `docs/roadmaps/distances_geodesic_engine.md`

Status (summary):

* DistanceEngine extracted and parity verified
* UI progress decoupled via sink interface
* Headless CLI exists and prints deterministic metrics
* Golden output comparison in place
* Micro-benchmarking guardrails active

---

### WS2 — Graph as Façade / Coordinator (DONE)

Goal: make `Graph` orchestration glue rather than algorithm host.

* Detailed plan: `docs/roadmaps/roadmap_ui_graph_facade.md`

Status (summary):

* Façade contract defined
* Engine boundary tightened
* `graph.cpp` mechanically dismantled
* UI access restricted to façade methods
* Algorithm/UI separation enforced (no UI construction in algorithm slices)

`Graph` now acts as:

* State holder + invariants guardian
* Explicit façade API for UI and CLI
* Delegator to algorithm slices
* Central UI signal coordinator

All changes verified via golden comparisons and performance benchmarks.

WS2 is structurally complete.

---

### WS3 — Domain Model Split (SKELETON)

Goal: establish a domain model that does not depend on Qt UI concerns.

* Plan: `docs/roadmaps/roadmap_domain_model_split.md`

Sequencing note:
WS3 assumes WS2 façade boundaries are stable.

---

### WS4 — IO / Parser Refactor (NEXT)

Goal: move file loading toward a clean IO layer and reduce Qt signal entanglement.

* Plan: `docs/roadmaps/roadmap_io_parser_refactor.md`

Next focus:

* Clarify parser responsibilities
* Reduce Qt signal coupling in load flow
* Prepare clean IO boundary before WS3 domain split

WS4 is the next active workstream.

---

### WS5 — Matrices / Linear Algebra Modernization (SKELETON)

Goal: isolate matrix constructs and computations into a coherent module.

* Plan: `docs/roadmaps/roadmap_matrices_modernization.md`

---

### WS6 — Testing + CI + Regression Baselines (SKELETON)

Goal: make it hard to regress outputs silently.

* Plan: `docs/roadmaps/roadmap_testing_ci_regression.md`

---

## Suggested Target Folder Layout (End State)

Conceptual target, not an immediate rewrite:

```
domain/
 ├── model/
 ├── algorithms/
 ├── matrices/
 ├── io/
 └── services/
```

---

## How to Contribute

* Pick a workstream document in `docs/roadmaps/`
* Follow its work rules
* Keep commits small and verifiable
* Preserve golden and performance baselines
