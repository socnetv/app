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

The `Graph` object is a façade and state coordinator — not a monolith. Algorithm logic lives in dedicated slices under `src/graph/`. A headless CLI regression harness (8 kernels) guards against silent regressions.

---

# Completed Workstreams

| WS   | Name                               | Status                       | Roadmap |
|------|-------------------------------------|-------------------------------|---------|
| WS1  | Algorithm extraction                | ✔ complete                    | [`roadmap_ws1_distances_geodesic_engine.md`](roadmaps/roadmap_ws1_distances_geodesic_engine.md) |
| WS2  | Graph façade                        | ✔ complete                    | [`roadmap_ws2_ui_graph_facade.md`](roadmaps/roadmap_ws2_ui_graph_facade.md) |
| WS3  | Architecture & Performance          | ✔ complete (v3.6/v3.7)        | [`roadmap_ws3_architecture_performance.md`](roadmaps/roadmap_ws3_architecture_performance.md) |
| WS4  | IO / Parser modernization           | ✔ complete                    | [`roadmap_ws4_io_parser_refactor.md`](roadmaps/roadmap_ws4_io_parser_refactor.md) |
| WS5  | Matrices Modernization              | ✔ complete (v3.7)             | [`roadmap_ws5_matrices_modernization.md`](roadmaps/roadmap_ws5_matrices_modernization.md) |
| WS9  | Graph exploration & data workflows  | ✔ shipped v3.5/v3.6           | [`roadmap_ws9_graph_exploration.md`](roadmaps/roadmap_ws9_graph_exploration.md) |
| WS14 | Logging Cost & Release-Build Hygiene| ✔ complete (v3.7, #268)       | [`roadmap_ws14_logging_cost.md`](roadmaps/roadmap_ws14_logging_cost.md) |

---

# Active Workstreams

See "Priorities" further down for the current ranking — no single workstream is pinned as "the"
active focus right now.

## WS6 — Testing / CI / Regression (SUPPORTING, ongoing)

Roadmap: [`docs/roadmaps/roadmap_ws6_testing_ci_regression.md`](roadmaps/roadmap_ws6_testing_ci_regression.md)

Expand golden baselines, dataset coverage, and benchmarking. Supports all other workstreams.

---

## WS7 — MainWindow Decomposition (LATER)

Break `MainWindow` into smaller, focused UI components. No UX changes — pure structural cleanup. Deferred until the UI stabilizes post-WS9.

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

Ongoing GraphicsWidget work, separate from WS3. Phase 1 (GraphicsWidget
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
possible to root-cause #260. Seven more commands shipped (#262: `relation`, `unilateral`, `erdos`,
`save`, `add-node`, `add-edge`, `add-relation`), built specifically to stress-test WS3's
edge-visibility batching change end-to-end. `distances`/`distances centralities` added during WS14
to get real GUI-side before/after timing evidence instead of assuming the CLI number transfers.

---

## WS13 — Undo/Redo

Roadmap: [`docs/roadmaps/roadmap_ws13_undo_redo.md`](roadmaps/roadmap_ws13_undo_redo.md)

General undo/redo for graph-mutating operations (#31), extending the `GraphVisibilitySnapshot`
pattern already shipped for filter/visibility undo (WS9) to structural mutations and attribute
edits. Previously mis-tracked as a WS3 dependency ("needs a stable domain model first") — that
dependency was checked and found fictional. Just created, not started; open design questions listed
in the roadmap.

---

## WS15 — App Responsiveness Contract (Dispatch, Cancellation, Busy-Guard & Parallelization)

Roadmap: [`docs/roadmaps/roadmap_ws15_cancellation_progress_unification.md`](roadmaps/roadmap_ws15_cancellation_progress_unification.md)

Split off from WS5 (A5's cancellation plumbing turned out to be inert in practice — a threading/
signal-delivery bug, not a `Matrix` bug) and WS7 (the progress-dialog-duplication finding, which
shares the same root cause), then reorganized (2026-08-05) around an explicit 4-property
"responsiveness contract" — non-blocking dispatch, working cancellation, busy-guard coverage, and
internal parallelization where the algorithm allows it — checked independently per operation, so
fixing one property doesn't get mistaken for having fixed all four (as happened with #52's
"comprehensive" cancel fix in v3.4, which silently regressed under `runGraphOperationAsync`).

---

# Priorities

1. **WS15** — app responsiveness contract. P1-P3 done and live-verified; P4's parallelization audit
   done, implementation not started; Finding 8 open.
2. **WS6** — regression safety (ongoing support — continuously active underneath every other
   workstream, not "next in queue").
3. **WS10** — GraphicsWidget canvas rendering & features. Phase 1 (#250), #260, and the
   rendering-perf regression kernel (WS6.6) all shipped; the rest of the Performance/Feature
   checklists remain scoped but not prioritised yet.
4. **WS7** — MainWindow decomposition. Solid milestone roadmap (MW1–MW7) exists; zero code written
   yet.
5. **WS8** — IO layer stabilization. Roadmap scoped; zero code written yet.
6. **WS11** — algorithm additions. Just created; not prioritised yet, no code written.
7. **WS12** — CLI scripting mode. Two steps shipped (#261, #262); further commands backlog, not
   prioritised.
8. **WS13** — undo/redo. Just created; not prioritised yet, no code written.

---

# Target Architecture

No standing plan to split `Graph` into a separate domain-model layer — see
`roadmap_ws3_architecture_performance.md` for why that turned out unnecessary. `Graph` stays the
façade indefinitely. Future structural change should follow WS3 M1's pattern: a specific, measured
problem found first, architecture second.

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
