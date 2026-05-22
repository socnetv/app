# ARCHITECTURAL REFACTORING ROADMAP

This document describes the **architectural direction of SocNetV**: where we are, where we are going, and how we get there.

Detailed workstream execution plans live in:

```
docs/roadmaps/
```

---

# Where We Are

SocNetV has a layered, modular architecture:

```
UI (MainWindow + dialogs + graphics)
↓
Graph (thin façade / coordinator)
↓
Algorithm slices / engines
├── distances / centralities
├── clustering / cohesion
├── reachability / connectivity
├── filters / subgraphs
├── layouts / generators
├── IO (Parser → IGraphParseSink → Graph)
└── matrices
```

The `Graph` object is a façade and state coordinator — not a monolith. Algorithm logic lives in dedicated slices under `src/graph/`. A headless CLI regression harness (7 kernels) guards against silent regressions.

---

# Completed Workstreams

| WS  | Name                              | Status         |
|-----|-----------------------------------|----------------|
| WS1 | Algorithm extraction              | ✔ complete     |
| WS2 | Graph façade                      | ✔ complete     |
| WS4 | IO / Parser modernization         | ✔ complete     |
| WS9 | Graph exploration & data workflows| ✔ shipped v3.5/v3.6 |

Details: [`docs/roadmaps/roadmap_graph_exploration.md`](roadmaps/roadmap_graph_exploration.md)

---

# Current Focus

**Bug fixes and issue triage** — closing long-standing GitHub issues before the next feature workstream begins.

All changes go through the WS6 regression harness (CLI golden compares + benchmarks).

---

# Upcoming Workstreams

## WS6 — Testing / CI / Regression (SUPPORTING, ongoing)

Roadmap: [`docs/roadmaps/roadmap_testing_ci_regression.md`](roadmaps/roadmap_testing_ci_regression.md)

Expand golden baselines, dataset coverage, and benchmarking. Supports all other workstreams.

---

## WS3 — Domain Model Split (MID-TERM)

Roadmap: [`docs/roadmaps/roadmap_domain_model_split.md`](roadmaps/roadmap_domain_model_split.md)

Introduce a domain model independent of the Graph façade:

```
model (nodes, edges, relations)
vs
algorithms / services / caches
```

Proceed incrementally. Let real usage (post-WS9 patterns) drive abstraction boundaries.

---

## WS7 — MainWindow Decomposition (LATER)

Break `MainWindow` into smaller, focused UI components. No UX changes — pure structural cleanup. Deferred until the UI stabilizes post-WS9.

---

## WS5 — Matrices Modernization

Roadmap: [`docs/roadmaps/roadmap_matrices_modernization.md`](roadmaps/roadmap_matrices_modernization.md)

Isolate and modernize matrix operations as a self-contained subsystem.

---

## WS8 — IO Layer Stabilization

Simplify the parser/IO dispatch model:

- `FormatHandler` abstraction
- cleaner format registration
- easier extensibility

---

# Priorities

0. **Bug fixes & issue triage** ← current
1. **WS6** — regression safety (ongoing support)
2. **WS3** — domain model split
3. **WS7** — MainWindow decomposition
4. **WS5** — matrices
5. **WS8** — IO

---

# Target Architecture (Long-Term)

```
domain/
├── model/        ← nodes, edges, relations
├── algorithms/   ← engines and slices
├── matrices/
├── io/
└── services/
```

`Graph` remains a façade coordinating these layers during transition.

---

# Guiding Principles

- Preserve functionality and numeric results
- Preserve performance — no regressions
- Keep changes incremental: **build → run → compare**
- Prefer vertical slices over large rewrites
- Let real usage drive abstraction boundaries
- Avoid premature modularization

---

# Contribution Workflow

1. Identify the relevant workstream
2. Follow its roadmap in `docs/roadmaps/`
3. Keep commits small and focused
4. After each structural change:

```
build
./scripts/run_golden_compares.sh
./scripts/run_benchmarks.sh
```

Golden baselines and benchmarks must remain stable.
