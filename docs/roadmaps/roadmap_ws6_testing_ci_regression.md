# Testing / CI / Regression Roadmap (WS6)

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

#### Known: parallel reduction is FP non-deterministic, with ~2.9× tolerance headroom

Found 2026-07-30 while measuring something else. **Not a live failure — a thin margin worth knowing
before baselines grow.**

The same binary produces a *different* result on every run. Measured: 40 consecutive runs of the
`StokmanZiegler_Netherlands` weighted case (`-c 1 -w 1 -x 1`) produced **40 distinct outputs**.
Cause is floating-point summation order in WS3 M1's parallel BC/SC reduction — thread scheduling
decides the accumulation order, and FP addition isn't associative. Affects `BC`, `SBC`, `PC`, `SPC`
and `metrics.avg_distance`.

This is currently absorbed by the harness: per-node fields compare with a **relative** tolerance of
`1e-15` (`kernel_distance_v1.cpp:186` → `almostEqual()`, `cli_common.cpp:129`), and graph-level
`avg_distance` via `cmpNumStrTol(..., 1e-15)`. All 40 golden compares passed.

The concern is the margin. Worst observed relative spread across 25 runs: **3.4e-16** against a
`1e-15` tolerance — **2.9× headroom**, no more. That margin shrinks as thread count and accumulation
chain length grow, and WS6.2 explicitly plans to add *larger* datasets. A `geom.net`-scale baseline
(7343 nodes) could plausibly exceed `1e-15` and turn the suite intermittently red, which is the
worst possible failure mode for a regression harness — flaky, unreproducible, and easy to
misattribute to whatever change happened to be in flight.

Also worth correcting: `cli_common.cpp:46` documents `d2s()` as *"Deterministic string for golden
compare (avoid float parse/format differences)"* — the formatting is deterministic, but the value
being formatted is not. The comment currently implies a guarantee the engine stopped providing when
the source loop was parallelised.

#### This is a harness-precision question, not a results-precision one

Worth stating plainly, because it decides the fix: **no user ever sees the digits that vary.**
SocNetV's reports already render real numbers at **6 significant digits** — `m_reportsRealPrecision`
defaults to 6 (`graph.cpp:112`, mirrored by `appSettings["initReportsRealNumberPrecision"]`,
`mainwindow.cpp:514`) and is user-configurable via Settings; every report writer applies it through
`outText.setRealNumberPrecision(m_reportsRealPrecision)`. A drift at the 16th significant digit is
invisible in every report SocNetV produces, and no researcher is interpreting a centrality score
past 2–3 digits anyway.

So the 17-digit `d2s()` serialization and the `1e-15` compare tolerance exist **only inside the
golden harness**. They are a regression detector, not a result. The harness is currently about
**eleven orders of magnitude stricter than the product's own output resolution** — which is why it
is the harness, not the engine, that should give.

**Choosing a tolerance.** Two bounds, and a lot of room between them:

- *Floor* — must sit above FP noise: `3.4e-16` today, and rising with thread count and network size.
- *Ceiling* — must sit below the smallest difference that indicates a real defect. A genuine
  semantic regression (wrong formula, wrong normalisation, wrong traversal, off-by-one in a path
  count) moves the leading significant digits — relative differences of `1e-3` and up, not `1e-15`.
  A useful anchor: a difference invisible at the product's own 6-digit display is arguably not an
  observable regression at all.

Something around `1e-9` sits ~7 orders above the noise floor and ~3 below display resolution, and
would catch every class of real regression the harness is actually there to catch. The exact figure
should be argued and recorded in the code, not just bumped until the suite goes quiet.

**Two things not to change while doing this:**

- **Keep `d2s()` at 17 digits.** The baseline file should losslessly record what actually happened;
  rounding at serialization creates boundary artifacts (`0.4999995` and `0.5000005` round apart
  while being equal to within any sane tolerance). Compare-time tolerance is strictly more robust
  than round-then-compare.
- **Keep integer-valued results on exact comparison.** `diameter`, `disconnected_pairs`, component
  IDs, triad/clique census counts and similar go through `cmpInt`, not `almostEqual`. Those are
  counts, they are exact, and an off-by-one there is a real bug — no tolerance should ever be
  applied to them. The split already exists; preserve it.

**The alternative — determinism by construction — is not free.** Slots are assigned by *thread
identity* (`distance_engine.cpp:487-499`): a `QHash<Qt::HANDLE,int>` hands each OS thread a slot on
first entry, and `QtConcurrent::blockingMap` distributes sources across threads however the
scheduler happens to. The final reduction over `allStates` *is* already fixed-order — the drift
comes from the *partitioning*, i.e. which sources' BC/SC contributions get grouped into which
partial accumulator, not from the merge. Making that deterministic means statically partitioning
`sources` into P fixed chunks and mapping over chunk descriptors instead of over individual sources,
which trades away `blockingMap`'s dynamic load balancing — and sources differ substantially in cost,
so that could eat into M1's measured 2.7×–8.3×. **Unmeasured; do not assume either way.** Given that
the tolerance argument above resolves the problem without touching the engine at all, this is the
fallback, not the first move.

**Suggested order:** widen the tolerance with the reasoning recorded → then measure the actual
spread at `geom.net` scale (7343 nodes) to confirm the new margin holds where WS6.2 is heading.

Reproduce: run the same `--dump-json` invocation N times and hash the output with `_ms` fields
stripped.

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

**Gap found 2026-07-29, while shipping WS3 M2's batched-signal work:** the golden harness has no
*correctness* coverage of the canvas either, not just no performance coverage. Verifying that
`GraphicsWidget::setEdgesVisibilityBatch()` actually left the right set of edges visible/hidden
after a relation switch or a unilateral-edge toggle had no automated check available — the only
verification was live manual testing (`docs/roadmaps/roadmap_ws12_cli_scripting_mode.md`'s
`--interactive-script` mechanism, built partly for this reason). A future canvas kernel should
assert on actual `GraphicsWidget` *state* after a fixed operation sequence (which edges/nodes end
up visible, positioned where expected, etc.), not just timing — same offscreen-rendering
constraint as the performance kernel above, but a state-comparison kernel rather than a
timing-threshold one. Could plausibly share the same kernel/harness scaffolding as WS6.6, or be a
sibling kernel (`kernel_render_state_v9`-shaped) — not scoped in detail yet.

---

## Notes

- Baseline regeneration should be treated as exceptional.
- Any “FAIL” in benchmarks must be investigated; if it is noise, prefer mitigation via more stable measurement rather than loosening thresholds by default.
- WS6 work should remain incremental: small changes, deterministic evidence, and consistent scripts.
```

### One small thing to fix later (not required now)

Your benchmark script output still shows `BUILD_TYPE=Debug` even when you run the Release binary. That’s a script-reporting detail (not a functional bug), but it can confuse future contributors. It’s a good tiny WS6 task.

If you paste me your current `docs/roadmaps/roadmap_ws6_testing_ci_regression.md` file path/contents (if it differs from the skeleton you showed), I can also produce a `diff`-style patch, but you don’t need to — the above is ready to drop in.

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

### Shipped-dataset roundtrip script fails on a custom-delimiter fixture (#256) ✅ Fixed

`run_io_roundtrip_shipped_datasets.sh` globs every file under `src/data` and loads each one via
the CLI using its default delimiter (`" "`, set in `socnetv_cli.cpp`'s `delimOpt`), with no
per-file override table. `src/data/TinyAdj_Dir_N3_E4_clucof.adj` used `|` as its column delimiter
— likely added alongside the directed clustering-coefficient fix (#58, `81e82a46`) to exercise the
GUI's Import-dialog custom-delimiter option — so it failed under the script's blanket
space-delimiter assumption. This went unnoticed because the script isn't part of the required
regression gate (see the three scripts listed under "Regression discipline" in
`README_DEVELOPER_NOTES.md`) or CI.

**Fixed (2026-07-29, `802b0097`)** by reformatting the fixture file itself to plain space
delimiters, matching every other `.adj` file in `src/data/` — simpler than either of the two
alternatives originally considered here (a per-file delimiter override table, or promoting the
script into CI), since the nonstandard delimiter wasn't actually load-bearing for what the fixture
tests (directed clustering coefficient). Promoting this script into the required gate / CI is still
open, now unblocked.

### Perf benchmark baselines predated the M1 DistanceEngine speedup — ✅ Fixed on all three sets

All three committed baselines predated M1's DistanceEngine parallelization
(`7900809e`/`11da8ef4`, 2026-05-26, the same day v3.6 shipped) by months:
`scripts/perf_baselines/macos-arm64/perf_expected.env` (2026-03-03, `ada8e613`),
`scripts/perf_baselines/macos-m5/perf_expected.env` (2026-02-17, `5661b7eb`), and
`scripts/perf_baselines/linux-x86_64/perf_expected.env` (2026-03-03, `013c05ce`). `run_benchmarks.sh`
still ran and reported "OK" against these stale baselines, but the "beats baseline by 30–70%"
results on the DistanceEngine-heavy benchmarks (EIES48, BA500, DIST_GRAPHML) weren't drift or
noise — they were exactly M1's real 2.7×–8.3× speedup being measured against a pre-M1 floor. A
regression eating into half of M1's gain would still have silently passed as "faster than
baseline." Found when a benchmark run was (wrongly) cited as regression evidence for a small WS3
M2 change; the actual evidence for that change was architectural (equivalent lookup complexity),
not the benchmark comparison, which prompted checking why the margins looked so large.

**`macos-arm64` and `macos-m5` fixed (2026-07-29)**, both from this machine: checked out the `v3.6`
tag directly (a clean release point, not just "whatever HEAD happens to be today"), built, and ran
`run_benchmarks.sh --record` there — `macos-arm64` via the script's own `uname`-based
auto-detection (which can only ever resolve to `macos-arm64` on any Apple Silicon Mac, M1 through
M5 — `uname -m` doesn't report chip generation), `macos-m5` via an explicit
`BENCH_BASELINE_SET=macos-m5 BENCH_RECORD_ALLOW_OVERRIDE=1` override for this specific chip.
Verified against current `develop`: every benchmark on both sets now lands within 1–5% of its
baseline (not 30–70% "faster"), confirming no regression from the WS3/WS10 work landed since v3.6,
and that future comparisons on this machine are meaningful again.

**`linux-x86_64` fixed (2026-07-29)**, from a 12-core Ryzen Linux x86_64 box: same method — a
temporary `git worktree` checked out at the `v3.6` tag (kept `develop` untouched), built with
`-DBUILD_CLI=ON` against Qt 6.8.3 (`/home/dimitris/Qt/6.8.3/gcc_64`), then
`run_benchmarks.sh --record` there. `auto_baseline_set` resolved to `linux-x86_64` on its own
(`uname` → `Linux`/`x86_64`), matching the existing baseline dir, so no `BENCH_BASELINE_SET`
override was needed this time. Recorded numbers dropped far more than the 2.7×–8.3× seen on
macOS (e.g. `DIST_GRAPHML` ~10×) — on top of the M1 speedup, the stale baseline was almost
certainly recorded on a slower/different Linux box than this one, so part of the delta is just
"first recording on this machine," not solely M1's contribution. Verified against current
`develop`: every benchmark lands within 0–9% of the new baseline (not 30–70% "faster"),
confirming no regression from the WS3/WS10 work landed since v3.6, and that future comparisons on
this machine are meaningful again.
