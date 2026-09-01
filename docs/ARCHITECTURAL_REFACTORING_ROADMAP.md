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

The `Graph` object is a façade and state coordinator — not a monolith. Algorithm logic lives in dedicated slices under `src/graph/`. A headless CLI regression harness (9 kernels) guards against silent regressions.

---

# Completed Workstreams

| WS   | Name                               | Status                       | Roadmap |
|------|-------------------------------------|-------------------------------|---------|
| WS1  | Algorithm extraction                | ✔ complete                    | [`roadmap_ws1_distances_geodesic_engine.md`](roadmaps/roadmap_ws1_distances_geodesic_engine.md) |
| WS2  | Graph façade                        | ✔ complete                    | [`roadmap_ws2_ui_graph_facade.md`](roadmaps/roadmap_ws2_ui_graph_facade.md) |
| WS3  | Architecture & Performance          | ✔ complete (v3.6/v3.7)        | [`roadmap_ws3_architecture_performance.md`](roadmaps/roadmap_ws3_architecture_performance.md) |
| WS4  | IO / Parser modernization           | ✔ complete                    | [`roadmap_ws4_io_parser_refactor.md`](roadmaps/roadmap_ws4_io_parser_refactor.md) |
| WS5  | Matrices Modernization              | ✔ complete (v3.7)             | [`roadmap_ws5_matrices_modernization.md`](roadmaps/roadmap_ws5_matrices_modernization.md) |
| WS14 | Logging Cost & Release-Build Hygiene| ✔ complete (v3.7, #268)       | [`roadmap_ws14_logging_cost.md`](roadmaps/roadmap_ws14_logging_cost.md) |
| WS16 | Report CSV Export                   | ✔ complete (v3.7, #113)       | [`roadmap_ws16_report_csv_export.md`](roadmaps/roadmap_ws16_report_csv_export.md) |
| WS7  | MainWindow Decomposition (MW0)      | ✔ complete (v3.7, #257)       | [`roadmap_ws7_mainwindow_decomposition.md`](roadmaps/roadmap_ws7_mainwindow_decomposition.md) |

---

# Active Workstreams

See "Priorities" further down for the current ranking — no single workstream is pinned as "the"
active focus right now.

## WS6 — Testing / CI / Regression (SUPPORTING, ongoing)

Roadmap: [`docs/roadmaps/roadmap_ws6_testing_ci_regression.md`](roadmaps/roadmap_ws6_testing_ci_regression.md)

Expand golden baselines, dataset coverage, and benchmarking, supporting every other workstream.

**Queued, deferred to post-v3.7**: WS6.8 — independently audit pre-existing golden baselines for
mathematical correctness (not just regression-stability). See the WS6 roadmap doc for why and how.

---

## WS8 — IO Layer Stabilization

Roadmap: [`docs/roadmaps/roadmap_ws8_io_layer_stabilization.md`](roadmaps/roadmap_ws8_io_layer_stabilization.md)

Consolidate per-format IO dispatch behind a single `FormatHandler` registry, replacing
hand-maintained per-format switch statements.

---

## WS10 — GraphicsWidget: Canvas Rendering & Features

Roadmap: [`docs/roadmaps/roadmap_ws10_graphicswidget_overhaul.md`](roadmaps/roadmap_ws10_graphicswidget_overhaul.md)

Ongoing GraphicsWidget canvas rendering and feature work, separate from WS3.

---

## WS9 — Graph Exploration & Data Workflows

Roadmap: [`docs/roadmaps/roadmap_ws9_graph_exploration.md`](roadmaps/roadmap_ws9_graph_exploration.md)

Core (filtering, subgraph extraction, table/CSV/JSON data workflows) shipped v3.5–v3.6. Debt
backlog still open: tab-based multi-graph UI (#245), attribute transformations (#229), temporal
attributes/timeline (#222), dynamic networks (#25), multirelational node removal (#57).

---

## WS11 — Algorithm Additions

Roadmap: [`docs/roadmaps/roadmap_ws11_algorithm_additions.md`](roadmaps/roadmap_ws11_algorithm_additions.md)

New analysis algorithms against the existing `src/graph/` slice architecture — centrality,
cohesion, clustering, structural equivalence.

---

## WS12 — CLI Interactive/Scripting Mode

Roadmap: [`docs/roadmaps/roadmap_ws12_cli_scripting_mode.md`](roadmaps/roadmap_ws12_cli_scripting_mode.md)

Drive SocNetV from the command line without manual clicking — scripted demos, automation, and
reproducible profiling/testing of GUI flows `socnetv-cli` can't reach.

---

## WS13 — Undo/Redo

Roadmap: [`docs/roadmaps/roadmap_ws13_undo_redo.md`](roadmaps/roadmap_ws13_undo_redo.md)

General undo/redo for graph-mutating operations, extending WS9's `GraphVisibilitySnapshot` pattern
to structural mutations and attribute edits.

---

## WS15 — App Responsiveness Contract (Dispatch, Cancellation, Busy-Guard & Parallelization)

Roadmap: [`docs/roadmaps/roadmap_ws15_cancellation_progress_unification.md`](roadmaps/roadmap_ws15_cancellation_progress_unification.md)

App responsiveness contract — non-blocking dispatch, working cancellation, busy-guard coverage, and
internal parallelization — checked independently per operation, so fixing one property can't be
mistaken for having fixed all four.

---

# Priorities

1. **WS15** — app responsiveness contract. P1-P3 done and live-verified; P4's parallelization audit
   done, implementation started.
2. **WS6** — regression safety (ongoing support — continuously active underneath every other
   workstream, not "next in queue").
3. **WS10** — GraphicsWidget canvas rendering & features. Phase 1 (#250), #260, and the
   rendering-perf regression kernel (WS6.6) all shipped; the rest of the Performance/Feature
   checklists remain scoped but not prioritised yet.
4. **WS8** — IO layer stabilization. Roadmap scoped; zero code written yet.
5. **WS11** — algorithm additions. Started: #7 and #272 shipped; rest of the backlog not
   prioritised yet.
6. **WS12** — CLI scripting mode. Thirty-four commands shipped across several passes since
   #261/#262; further commands added on demand, not prioritised as a standing backlog.
7. **WS13** — undo/redo. Just created; not prioritised yet, no code written.
8. **WS9** — graph exploration debt backlog. Core shipped; five open issues (#245, #229, #222,
   #25, #57), none prioritised yet — #245 blocks on significant tab-UI infrastructure investment.

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
