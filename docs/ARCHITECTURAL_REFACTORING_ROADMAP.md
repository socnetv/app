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

The `Graph` object is a façade and state coordinator — not a monolith. Algorithm logic lives in dedicated slices under `src/graph/`. A headless CLI regression harness (10 kernels) guards against silent regressions.

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
| WS15 | App Responsiveness Contract         | ✔ complete (3.8-cycle)        | [`roadmap_ws15_cancellation_progress_unification.md`](roadmaps/roadmap_ws15_cancellation_progress_unification.md) |

---

# Active Workstreams

See "Priorities" further down for the current ranking — no single workstream is pinned as "the"
active focus right now.

## WS6 — Testing / CI / Regression (SUPPORTING, ongoing)

Roadmap: [`docs/roadmaps/roadmap_ws6_testing_ci_regression.md`](roadmaps/roadmap_ws6_testing_ci_regression.md)

Golden baselines, dataset coverage, and benchmarking, supporting every other workstream.

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

## WS17 — Bipartite / Two-Mode Network Analysis

Roadmap: [`docs/roadmaps/roadmap_ws17_bipartite_analysis.md`](roadmaps/roadmap_ws17_bipartite_analysis.md)

Persistent bipartite/partition model, two-mode GraphML/Pajek round-trip and `.2sm` export,
two-mode layouts, bipartite generators, Robins-Alexander clustering, bipartite matching
(Hopcroft-Karp). No external dependencies — hand-rolled, consistent with the rest of the codebase.

---

## WS18 — Signed Network Analysis / Structural Balance

Roadmap: [`docs/roadmaps/roadmap_ws18_signed_network_analysis.md`](roadmaps/roadmap_ws18_signed_network_analysis.md)

First-class support for signed networks (edge sign, not just magnitude). P0-P3 shipped in v3.8:
negative-weight parser support, guarding existing distance-based measures against negative
weights, Bellman-Ford/Johnson's-algorithm negative-weight-safe shortest paths, and signed-specific
centrality (Signed Degree, PN Centrality). P4 (Heider/Cartwright-Harary structural balance
analysis on triads, building on the existing MAN triad census, #305) is implemented on develop,
with GUI menu/Control Panel wiring (#307's non-layout scope) - both still open pending the 3.9
release. #304 (clusterizable / two-faction test, the strong-theorem follow-on) is scoped, in
progress; #303 (directed-graph balance semantics) and #307's remaining balance-driven-layout
exploration are not yet started.

---

## WS19 — AppStream Identity & Packaging Names (transient, 3.9)

Roadmap: [`docs/roadmaps/roadmap_ws19_appstream_identity.md`](roadmaps/roadmap_ws19_appstream_identity.md)

Standard application ID (`org.socnetv.SocNetV`) for the AppStream metadata file, desktop file,
icon and window association, correct metadata license, and Flathub readiness (#309, prerequisite
for #167) — without breaking existing downstream packages (Debian, Fedora, OBS, AppImage). Planned,
not started; blocked on downstream packagers switching to glob-based file lists first.

---

# Priorities

No workstream is pinned as "the" active focus — work happens on whichever's issue is picked up
next. `gh issue list --milestone <3.8|3.9|4.0>` is the authoritative view of what's queued where —
this is a workstream-level summary of that picture, not a ranking, and not a live issue count
(check the tracker directly for current numbers rather than trusting a snapshot here).

**v3.8 shipped.** WS6 (regression safety) runs continuously underneath every other workstream.
WS18 (signed networks) — see its own entry above for current status, not duplicated here.

**3.9 packaging:** WS19 (#309) — transient, coordinated with downstream packagers before any
installed file is renamed.

**Scoped, not started:** WS8 (IO layer stabilization/`FormatHandler` registry) — roadmap exists,
zero code written.

**Ongoing, demand-driven (no fixed end state):** WS10 (canvas rendering — Phase 1/#250/#260
shipped, rest of the checklist open-ended), WS11 (algorithm additions — #7/#272 shipped, backlog
open-ended), WS12 (CLI scripting — 34 commands shipped, more added on demand), WS9's debt backlog
(#229, #25, #57 — core already shipped v3.5-v3.6).

**Major/4.0-scoped, larger lifts, none started:** WS13 (undo/redo, #31), WS9's #245 (tab-based
multi-graph UI) and #222 (temporal attributes/timeline), WS17 (bipartite/two-mode analysis, #282),
WS11's #3 (cohesive subgroups — n-cliques/n-clans/k-plexes) and #181 (structural equivalence —
MDS/blockmodelling/CONCOR) — each its own standalone workstream investment, not a small patch.

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
