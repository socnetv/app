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

- clustering metrics / coefficients
- random network generators (deterministic via fixed seeds)
- layout / visualization computations runnable headlessly (compute-only; no QtWidgets/QtCharts)
- additional analysis workflows that users typically trigger from UI

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

## Notes

- Baseline regeneration should be treated as exceptional.
- Any “FAIL” in benchmarks must be investigated; if it is noise, prefer mitigation via more stable measurement rather than loosening thresholds by default.
- WS6 work should remain incremental: small changes, deterministic evidence, and consistent scripts.
```

### One small thing to fix later (not required now)

Your benchmark script output still shows `BUILD_TYPE=Debug` even when you run the Release binary. That’s a script-reporting detail (not a functional bug), but it can confuse future contributors. It’s a good tiny WS6 task.

If you paste me your current `docs/roadmaps/roadmap_testing_ci_regression.md` file path/contents (if it differs from the skeleton you showed), I can also produce a `diff`-style patch, but you don’t need to — the above is ready to drop in.
