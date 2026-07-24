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

**WS3 — Architecture & Performance.**

Phases 1 and 2 (DistanceEngine parallelisation) shipped in v3.6 (2.7×–8.3× speedup).
Next: dead code cleanup, near-term DistanceEngine feature deliverables, and bug fixes.
See [`docs/roadmaps/roadmap_architecture_performance.md`](roadmaps/roadmap_architecture_performance.md).

---

# Active Workstreams

## WS3 — Architecture & Performance ← **ACTIVE**

Roadmap: [`docs/roadmaps/roadmap_architecture_performance.md`](roadmaps/roadmap_architecture_performance.md)

Introduce a domain model independent of the Graph façade:

```
model (nodes, edges, relations)
vs
algorithms / services / caches
```

**First execution:** extract per-source SSSP scratch from `Graph`/`GraphVertex` into
`PerSourceScratch`; parallelise `DistanceEngine` source loop. Proceed incrementally;
every phase is validated by the WS6 regression harness before the next begins.

---

## WS6 — Testing / CI / Regression (SUPPORTING, ongoing)

Roadmap: [`docs/roadmaps/roadmap_testing_ci_regression.md`](roadmaps/roadmap_testing_ci_regression.md)

Expand golden baselines, dataset coverage, and benchmarking. Supports all other workstreams.

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

## WS10 — Canvas Rendering Performance

Roadmap: [`docs/roadmaps/roadmap_graphicswidget_overhaul.md`](roadmaps/roadmap_graphicswidget_overhaul.md)

Ongoing GraphicsWidget/canvas rendering performance work, separate from WS3's domain-model focus.
Phase 1 (GraphicsWidget Performance and Code Quality Overhaul, #250) shipped: correctness fixes,
hot-path allocation/scan reductions, structural changes, full documentation pass. Future work
(rendering-cost reduction, node-selection hot path, bulk-operation batching, a rendering-performance
regression kernel) is scoped but not started.

---

# Priorities

1. **WS3** — architecture & performance / DistanceEngine parallelisation ← **current**
2. **WS6** — regression safety (ongoing support)
3. **WS7** — MainWindow decomposition
4. **WS5** — matrices (receives the M1-continuation APSP migration from WS3)
5. **WS8** — IO
6. **WS10** — canvas rendering performance (Phase 1 shipped; future work not yet prioritised)

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
