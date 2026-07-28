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

Details: [`docs/roadmaps/roadmap_ws9_graph_exploration.md`](roadmaps/roadmap_ws9_graph_exploration.md)

---

# Active Workstreams

Current focus is marked `← ACTIVE` below; see "Priorities" further down for the full ranking.

## WS3 — Architecture & Performance ← **ACTIVE**

Roadmap: [`docs/roadmaps/roadmap_ws3_architecture_performance.md`](roadmaps/roadmap_ws3_architecture_performance.md)

Introduce a domain model independent of the Graph façade:

```
model (nodes, edges, relations)
vs
algorithms / services / caches
```

**M1 shipped** (v3.6): per-source SSSP scratch extracted into `PerSourceScratch`;
`DistanceEngine` source loop parallelised (2.7×–8.3× speedup). **Next: M2** — introduce
`GraphModel`, currently in design. Proceed incrementally; every milestone is validated by the
WS6 regression harness before the next begins.

---

## WS6 — Testing / CI / Regression (SUPPORTING, ongoing)

Roadmap: [`docs/roadmaps/roadmap_ws6_testing_ci_regression.md`](roadmaps/roadmap_ws6_testing_ci_regression.md)

Expand golden baselines, dataset coverage, and benchmarking. Supports all other workstreams.

---

## WS7 — MainWindow Decomposition (LATER)

Break `MainWindow` into smaller, focused UI components. No UX changes — pure structural cleanup. Deferred until the UI stabilizes post-WS9.

---

## WS5 — Matrices Modernization

Roadmap: [`docs/roadmaps/roadmap_ws5_matrices_modernization.md`](roadmaps/roadmap_ws5_matrices_modernization.md)

Isolate and modernize matrix operations as a self-contained subsystem.

---

## WS8 — IO Layer Stabilization

Roadmap: [`docs/roadmaps/roadmap_ws8_io_layer_stabilization.md`](roadmaps/roadmap_ws8_io_layer_stabilization.md)

Consolidate per-format dispatch — confirmed distinct from WS4 (which addressed the mutation
contract and translation-unit separation, not dispatch): four separate switch statements across
`parser.cpp`, `graph_io.cpp`, and `mainwindow.cpp` currently hand-maintain the same per-format
metadata in sync, with no enforcement — a `FormatHandler` registry replaces all four.

---

## WS10 — GraphicsWidget: Canvas Rendering & Features

Roadmap: [`docs/roadmaps/roadmap_ws10_graphicswidget_overhaul.md`](roadmaps/roadmap_ws10_graphicswidget_overhaul.md)

Ongoing GraphicsWidget work, separate from WS3's domain-model focus. Phase 1 (GraphicsWidget
Performance and Code Quality Overhaul, #250) shipped: correctness fixes, hot-path allocation/scan
reductions, structural changes, full documentation pass. The canvas-clear performance issue (#260)
is also shipped. Future work covers both a Performance Checklist (rendering-cost reduction,
node-selection hot path, bulk-operation batching, a rendering-performance regression kernel) and a
Feature Checklist (new canvas-drawing capabilities, e.g. #22) — scope widened beyond pure
performance since both live in the same class and the same `QGraphicsScene`/`QGraphicsView`
machinery.

---

## WS11 — Algorithm Additions

Roadmap: [`docs/roadmaps/roadmap_ws11_algorithm_additions.md`](roadmaps/roadmap_ws11_algorithm_additions.md)

New analysis algorithms requested against the existing, stable `src/graph/` algorithm-slice
architecture — centrality (Katz, Bonacich Power), cohesion (cohesive subgroups, connectivity),
clustering (community detection beyond HCA), structural equivalence (MDS, blockmodelling, CONCOR).
Pure numerical/graph-theory implementation, not architecture — distinct from WS5 (matrix
storage/performance) and from the completed WS9. Just created, not started.

---

## WS12 — CLI Interactive/Scripting Mode

Roadmap: [`docs/roadmaps/roadmap_ws12_cli_scripting_mode.md`](roadmaps/roadmap_ws12_cli_scripting_mode.md)

Drive SocNetV from the command line without manual clicking, for reproducible profiling and testing
of GUI-triggered flows the headless `socnetv-cli` tool can't reach. First step shipped (#261:
`--encoding`, `--interactive-script` with `delay`/`new` commands) — was the tool that made it
possible to root-cause #260. Further commands (#262) are backlog, not yet scoped.

---

# Priorities

1. **WS3** — architecture & performance ← **current**. M1 shipped (v3.6); M2 (`GraphModel`) design
   drafted, not yet started. #263 (three remaining GUI-freeze cases) is a small mechanical
   follow-up, next up before M2.
2. **WS10** — GraphicsWidget canvas rendering & features. Phase 1 (#250) and #260 fully shipped;
   future rendering-cost and feature work scoped but not prioritised yet.
3. **WS6** — regression safety (ongoing support — continuously active underneath every other
   workstream, not "next in queue").
4. **WS5** — matrices. Receives the M1-continuation APSP migration from WS3.
5. **WS7** — MainWindow decomposition. Solid milestone roadmap (MW1–MW7) exists; zero code written
   yet.
6. **WS8** — IO layer stabilization. Roadmap scoped; zero code written yet.
7. **WS11** — algorithm additions. Just created; not prioritised yet, no code written.
8. **WS12** — CLI scripting mode. First step shipped (#261); further commands not prioritised yet.

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
