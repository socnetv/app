# Testing / CI / Regression Roadmap

## Goal
Prevent silent regressions during modernization.

## Current Reality
- Manual comparisons exist; headless CLI now prints metrics.

## Target Direction
- Golden outputs committed in-repo
- A deterministic comparison tool
- CI job that runs core datasets

## Milestones
- T1: Define output schemas (metrics + per-node vectors)
- T2: Add golden baselines for a small suite of datasets
- T3: Add comparison mode (fail on mismatch)
- T4: Integrate into CI (GitHub Actions)

## Work Rules
- Keep outputs stable (version schemas when changing format).

---

## Updated Current State (Post-WS1/WS2/WS4)

The regression harness is now a first-class part of the modernization effort.

### CLI façade + kernels

`socnetv-cli` is a thin façade (argument parsing + dispatch only).  
All deterministic logic lives in kernel translation units under:

```

src/tools/cli/kernels/

```

Current kernels:

```

kernel_distance_v1.cpp
kernel_reachability_v2.cpp
kernel_walks_v3.cpp
kernel_prominence_v4.cpp
kernel_io_roundtrip_v5.cpp
kernel_clustering_v6.cpp
kernel_connectivity_v7.cpp

```

The CLI supports:

- deterministic metric printing (`cli::printKV`)
- JSON dump mode (`--dump-json`)
- JSON compare mode (`--compare-json`)
- benchmarks (distance kernel via `--bench`)
- an IO roundtrip kernel (`--kernel io_roundtrip`)
- strict mode for timing guardrails (`--strict`, used by benchmarking scripts)

### Headless loading is unified and deterministic

Both GUI and CLI use the same IO mutation pipeline introduced in WS4:

```

Parser
↓
IGraphParseSink
↓
Graph

```

The CLI loads graphs through:

```

tools/headless_graph_loader.h

```

This loader blocks on:

- Preferred: `Graph::signalGraphLoaded`
- Fallback: `Parser::finished`

### Regression scripts (currently active)

The test harness is primarily exercised via scripts:

```

scripts/run_golden_compares.sh
scripts/run_benchmarks.sh
scripts/run_golden_io_roundtrip.sh
scripts/run_io_roundtrip_shipped_datasets.sh

```

Key properties:

- Golden comparisons enforce deterministic algorithm outputs and IO stability.
- Benchmarks enforce performance guardrails against per-platform baselines.
- IO roundtrip baselines are committed in-repo under:

```

src/tools/baselines/io_roundtrip/

```

### What is already implemented vs the original milestones

The original milestone plan is still valid, but several items are already implemented:

- **T1 (schemas):** JSON-based outputs exist and are baseline-locked via `--dump-json` + `--compare-json`.
- **T2 (golden suite):** Golden baselines exist for multiple kernels and multiple IO formats (including export-skipped locking where exporters are missing).
- **T3 (comparison mode):** JSON comparison exists (`--compare-json`) and is exercised by `run_golden_compares.sh`.
- **T4 (CI):** still pending (CI integration is the main remaining step).

---

## Next Steps (WS6 priority)

WS6 should prioritize expanding **headless feature coverage** and making the harness easier to run locally.
CI integration is explicitly a later step.

### WS6.1 — Expand CLI kernel coverage (UI-adjacent functionality, headless)

Goal:

Expose more “UI-visible” functionality through deterministic CLI kernels, so it becomes testable headlessly without the GUI.

Examples of high-value additions:

- random network generators (deterministic via fixed seeds)
- layout / visualization computations runnable headlessly (compute-only; no QtWidgets/QtCharts)
- additional analysis workflows that users typically trigger from UI
- **kernel_connectivity_v7** ✔ — weakly connected component count + per-node component IDs (#85):
  - `Graph::graphWeaklyConnectedComponents()` — BFS treating all edges as undirected (weak connectivity); caches count in `m_graphWeaklyConnectedComponents` and per-node IDs in `m_vertexComponentId`. Cache invalidated with `resetDistanceCentralityCacheFlags()`.
  - Three baselines committed: `TinyDisconnected_Undir_N6_E4` (3 components), `TinyDisconnected_Dir_N5_E3` (2 weak components), `TinyPath_N3_E2` (1 component / connected).
  - **Connectivity semantics table** (what the kernel computes and what the UI reports):

    | Graph type | Topology | Components | `connected` | UI message |
    |---|---|---|---|---|
    | Undirected | All nodes reachable | 1 | true | "connected (1 component)" |
    | Undirected | N isolated islands | >1 | false | "disconnected (N components)" |
    | Directed | Every pair has a directed path | 1 | true | "weakly connected (1 component)" |
    | Directed | A→B only (not B→A) | 1 | true | "weakly connected (1 component)" — one island, not strongly connected |
    | Directed | Two separate islands | >1 | false | "disconnected (N weakly connected components)" |

    **Design rationale:** `connected = (components == 1)` uses weak connectivity throughout. For directed graphs this is weaker than strong connectivity (all-pairs directed reachability), but it answers the practical "how many islands?" question consistently for both directed and undirected networks. Strong connectivity remains available via `isConnected()` / SSSP.

- **kernel_attribute_import_v7** — CSV/JSON attribute import + export roundtrip (#227, #232):
  - Use `src/data/TinyDir_N2_E1_Attributes.graphml` as the seed graph (2 nodes, 1 edge; heterogeneous custom attrs `Age`/`Party` using `d1000+` keys — also covers #208 regression)
  - Export nodes and edges to CSV and JSON via `TableExport`
  - Mutate specific attribute values in the exported files
  - Re-import via `TableImport` + `Graph::vertexAttributesImport` / `edgeAttributesImport`
  - Assert: mutated attributes are present with correct values; native columns (Label, Size, Color, Weight) were routed to their proper setters; read-only columns were not stored as custom attributes; no duplicate column keys
  - Assert roundtrip fidelity: export without modification → re-import → golden compare (no change)
  - Save as GraphML and reload; assert attribute persistence survives the full pipeline

Rules:

- no behavior changes in existing functionality
- outputs must be deterministic (seeded where randomness exists)
- each new kernel must have a clear, versioned name (`kernel_<name>_vN`)
- add `--dump-json` and `--compare-json` support for each new kernel

Outcome:

More of the application’s “user-facing” features become regression-testable without the UI.

---

### WS6.2 — Systematically expand datasets and coverage

Goal:

Increase confidence by testing more networks and more edge cases in a structured way.

Approach:

- grow the dataset suite gradually
- include representative small/medium/large graphs
- include tricky parser edge cases per format (GraphML/DOT/Pajek quirks)
- where formats lack exporters, keep using export-skipped baseline locking
- prefer shipped datasets under `src/data` where possible; add external datasets only if licensing permits

Rules:

- add datasets incrementally
- baseline additions must be reviewed (do not bulk-regenerate)

---

### WS6.3 — Refactor the golden harness scripts for modularity

Problem:

`run_golden_compares.sh` currently does a lot and can be noisy.

Goal:

Split goldens into subscripts and keep a master runner that can execute:

- all suites (default)
- one suite
- a selected subset

Direction:

- create per-suite scripts (examples):
  - `scripts/goldens/golden_distance.sh`
  - `scripts/goldens/golden_reachability.sh`
  - `scripts/goldens/golden_walks.sh`
  - `scripts/goldens/golden_prominence.sh`
  - `scripts/goldens/golden_io_roundtrip.sh`
- add a master runner that supports:
  - `--list` (print available suites)
  - `--only <suite1,suite2,...>`
  - `--skip <suite1,suite2,...>`
  - default: run all

Outcome:

Faster local workflows and easier diagnosis when one suite fails.

---

### WS6.4 — Tighten determinism, measurement stability, and reporting

Goal:

Reduce false positives and reduce noise sensitivity, especially in IO benchmarks.

Possible improvements:

- clarify how BUILD_TYPE / configuration is detected and printed by scripts
- use median-of-N consistently where useful (especially IO load tests)
- separate IO load-time thresholds from compute-time thresholds where needed
- document baseline update rules (rare; only for real semantic fixes)

---

### WS6.5 — CI integration (LAST)

Goal:

Once local harness coverage is broad and stable, add CI checks to prevent regressions from landing silently.

Policy:

CI must not become the primary place where developers discover breakage.

CI should run a carefully chosen subset by default:

- build (Release preferred)
- a “fast” golden subset
- optionally a “fast” benchmark subset

Heavier suites can run nightly or on-demand.

---

### WS6.6 — Canvas rendering performance kernel (#240)

Goal:

The existing golden harness covers computation kernels (distance, prominence, clustering, etc.) but has no coverage of the graphics layer.  A headless rendering-performance kernel would let us catch regressions in `GraphicsWidget` / `GraphicsEdge` / `GraphicsNode` paint paths automatically.

Approach:

- Build a new CLI kernel (`kernel_render_perf_v8` or similar) that:
  - Loads a fixed, large-ish reference network (e.g. `Bernard_Killworth_Fraternity` or a synthetic dense graph) into a `QGraphicsScene` without showing a window (`QOffscreenSurface` / `QImage` render target)
  - Drives a fixed sequence of operations: full scene render, bulk node-size change, bulk edge-color change, simulated node drag (move one high-degree node N steps)
  - Measures wall-clock time for each operation and writes a JSON result (`"render_ms"`, `"bulk_node_size_ms"`, etc.)
- Baselines store the JSON with **timing upper bounds** (not exact values) so the comparison is a "must be faster than X ms" guard, not a brittle equality check
- `run_benchmarks.sh` gains a `--render` flag to include this kernel; CI keeps it off by default (GPU/display availability varies)

Why this matters:

`GraphicsWidget` bulk operations (`setEdgeArrowSize`, etc.) currently fire thousands of individual `prepareGeometryChange()` calls without batching.  Phase 1–5 of #240 fix the worst offenders, but without a regression kernel there is no automated guard to prevent the problems returning.

Rules:

- kernel must not open any visible window (offscreen rendering only)
- timing thresholds set conservatively (2× measured baseline on reference hardware) to tolerate CI noise
- add new threshold fields to the existing benchmark JSON schema

---

## Notes

- Baseline regeneration should be treated as exceptional.
- Any “FAIL” in benchmarks must be investigated; if it is noise, prefer mitigation via more stable measurement rather than loosening thresholds by default.
- WS6 work should remain incremental: small changes, deterministic evidence, and consistent scripts.
```

### One small thing to fix later (not required now)

Your benchmark script output still shows `BUILD_TYPE=Debug` even when you run the Release binary. That’s a script-reporting detail (not a functional bug), but it can confuse future contributors. It’s a good tiny WS6 task.

If you paste me your current `docs/roadmaps/roadmap_testing_ci_regression.md` file path/contents (if it differs from the skeleton you showed), I can also produce a `diff`-style patch, but you don’t need to — the above is ready to drop in.

### Continuous release page shows a stale "published" date (#255 follow-up)

`build-ci.yml`'s "Update continuous release description" step (`ubuntu-latest` job) `PATCH`es
the existing `continuous` release's body on every `[ci]` run, but never touches its
`published_at` timestamp — GitHub's releases page prominently displays that timestamp, so the
page can look outdated (currently stuck at 2025-02-25) even though the body text and uploaded
artifacts are fresh from the latest run. Proper fix is to delete and recreate the release each
run instead of updating in place, which needs care: the four OS jobs currently upload artifacts
to the existing `continuous` tag independently and in parallel, so the recreate step would need
to run once, before any of the parallel upload steps, to avoid a race. Separately, the same
script computes `commitMessage` (the commit's subject line) but never uses it in the description
text — worth including once this is revisited.

### Shipped-dataset roundtrip script fails on a custom-delimiter fixture (#256)

`run_io_roundtrip_shipped_datasets.sh` globs every file under `src/data` and loads each one via
the CLI using its default delimiter (`" "`, set in `socnetv_cli.cpp`'s `delimOpt`), with no
per-file override table. `src/data/TinyAdj_Dir_N3_E4_clucof.adj` uses `|` as its column delimiter
— likely added alongside the directed clustering-coefficient fix (#58, `81e82a46`) to exercise the
GUI's Import-dialog custom-delimiter option — so it fails under the script's blanket space-delimiter
assumption. This went unnoticed because the script isn't part of the required regression gate (see
the three scripts listed under "Regression discipline" in `README_DEVELOPER_NOTES.md`) or CI.

Two independent fixes, either is enough on its own:

- Give the script a small per-file delimiter override table (or a `.delimiter` sidecar convention)
  so intentionally-nonstandard fixtures don't read as failures.
- Promote the script into the required gate / CI now that it demonstrably catches real issues,
  once the delimiter mismatch above is resolved one way or another.
