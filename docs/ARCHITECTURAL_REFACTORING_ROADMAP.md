# ARCHITECTURAL_REFACTORING_ROADMAP

This document defines the **current architectural direction and active workstreams** of SocNetV.

Detailed execution plans live in:

```

docs/roadmaps/

```

---

# North Star

Evolve SocNetV from a Graph-centric monolith:

```

UI → Graph → everything

```

to a layered and extensible architecture:

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

---

# Current Architectural State

SocNetV has reached a stable baseline:

- Algorithms extracted into testable components (engines)
- Graph acts as façade and coordinator
- IO layer uses explicit mutation pipeline (Parser → Sink → Graph)
- Deterministic regression harness (CLI + golden baselines) is in place
- Parser is modularized by format

This allows safe, incremental evolution without breaking behavior.

---

# Guiding Principles

Non-negotiables:

- Preserve functionality and numeric results
- Preserve performance (no regressions)
- Keep changes incremental: **build → run → compare**
- Maintain deterministic behavior (CLI regression harness)

Engineering approach:

- Prefer vertical feature slices over large rewrites
- Reuse existing functionality where possible
- Avoid premature abstraction
- Let real usage drive architectural refinement

---

# Active Workstreams

---

## WS9 — Graph Exploration & Data Workflows (PRIMARY)

Defined in:

```

docs/roadmaps/roadmap_graph_exploration.md

```

### Goal

Make SocNetV usable for **real-world large networks**.

### Scope

WS9 consolidates the following GitHub issue tracks:

- **Visualization & Decluttering** → #209  
  (selection, ego networks, edge filtering, layouts)

- **Filtering & Subgraphs** → #215  
  (structural filters, attribute filters, subgraph workflows, queries)

- **Data Workflows** → #223  
  (attribute editing, tables, import/export, transformations)

Concrete sub-features are tracked in:

- #210–#214 (visualization)
- #216–#221 (filtering & querying)
- #224–#229 (data workflows)

WS9 acts as the **umbrella workstream** coordinating these efforts.


### Architectural Impact

Introduces a new conceptual layer:

```

Graph → Filter / Projection Layer → UI

```

Key properties:

- non-destructive (visibility-based)
- stateful (not dialog-driven)
- reusable across UI components

### Strategy

- Build incrementally on existing dialogs and systems
- Unify behavior over time (not via big rewrite)
- Respect current single-window architecture
- Prepare for future tab-based multi-graph UI

---

## WS6 — Testing / CI / Regression (SUPPORTING)

Defined in:

```

docs/roadmaps/roadmap_testing_ci_regression.md

```

### Goal

Prevent silent regressions during ongoing development.

### Responsibilities

- Maintain golden baselines
- Expand dataset coverage
- Ensure deterministic CLI behavior
- Support benchmarking

### Role

WS6 supports WS9 by ensuring all changes are:

- verifiable
- reproducible
- safe to refactor

---

## WS3 — Domain Model Split (MID-TERM)

Defined in:

```

docs/roadmaps/roadmap_domain_model_split.md

```

### Goal

Introduce a domain model independent of UI and Graph façade.

Target separation:

```

model (nodes, edges, relations)
vs
algorithms / services / caches

```

### Strategy

- Proceed incrementally
- Use WS9 requirements to guide abstraction boundaries
- Avoid blocking feature development

---

## WS7 — MainWindow Decomposition (LATER)

### Goal

Break down the MainWindow monolith into smaller UI components.

### Strategy

- Follow real UI evolution from WS9
- Avoid premature modularization
- No UX changes

---

## WS5 — Matrices Modernization

Defined in:

```

docs/roadmaps/roadmap_matrices_modernization.md

```

### Goal

Isolate and modernize matrix operations.

---

## WS8 — IO Layer Stabilization

### Goal

Refine and simplify the parser/IO architecture.

Possible direction:

- FormatHandler abstraction
- cleaner dispatch model
- easier extensibility

---

# Workstream Priorities

1. **WS9 — Graph Exploration & Data Workflows**
2. **WS6 — Testing / CI / Regression**
3. **WS3 — Domain Model Split**
4. **WS7 — MainWindow Decomposition**
5. **WS5 — Matrices**
6. **WS8 — IO**

---

# Workstream Relationships

```

WS9 (product evolution)
↓ supported by
WS6 (regression safety)

WS3 (domain model)
follows insights from WS9

WS7 (UI decomposition)
follows UI complexity from WS9

WS5 / WS8
independent infrastructure improvements

```

---

# Target Architecture (Long-Term)

```

domain/
├── model/
├── algorithms/
├── matrices/
├── io/
└── services/

```

Graph remains a façade coordinating these layers during transition.

---

# Contribution Workflow

When contributing:

1. Identify the relevant workstream
2. Follow its roadmap in `docs/roadmaps/`
3. Keep commits small and mechanical
4. After each change:

```

build
./scripts/run_golden_compares.sh
./scripts/run_benchmarks.sh

```

Golden baselines and performance must remain stable.

---

# Summary

SocNetV now evolves along two axes:

### Product Evolution

- Graph exploration
- Filtering and querying
- Structured data workflows

### Architectural Evolution

- Regression safety
- Domain model separation
- UI decomposition

Development should prioritize **real user value (WS9)** while maintaining architectural integrity through incremental refactoring.
