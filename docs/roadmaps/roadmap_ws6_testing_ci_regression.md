# Testing / CI / Regression Roadmap (WS6)

## Goal
Prevent silent regressions during modernization.

## Status

🚧 Ongoing, supporting workstream (no fixed end state — continuously active underneath every other
workstream). The original schema/golden-baseline/comparison-mode goals (see Current State below)
are all live; CI integration (WS6.5) is explicitly last and not started. WS6.1–WS6.4 in progress;
WS6.6 (canvas rendering-perf kernel) done; WS6.7 (matrix operation golden coverage,
`kernel_matrix_v8`) done — WS5's A3 is unblocked.

## Current State (Post-WS1/WS2/WS4)

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
kernel_matrix_v8.cpp

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

---

## Milestones

WS6 should prioritize expanding **headless feature coverage** and making the harness easier to run
locally. CI integration is explicitly a later step. Done items are listed first, for a quick
"what's already shipped" scan; the rest follow in rough priority order.

### WS6.6 — Canvas rendering performance kernel (#240) ✅ Done (v3.7)

Goal:

The existing golden harness covers computation kernels (distance, prominence, clustering, etc.) but has no coverage of the graphics layer.  A rendering-performance kernel would let us catch regressions in `GraphicsWidget` / `GraphicsEdge` / `GraphicsNode` paint paths automatically.

**Approach taken — deviates from the original plan below, for a concrete reason.** The original
idea (kept for context, see "Original approach" below) was a headless `socnetv-cli` kernel using
`QOffscreenSurface`/`QImage`. That doesn't fit this codebase: `socnetv-cli`
(`src/tools/socnetv_cli.cpp:28`) constructs a `QCoreApplication`, not a `QApplication`, and
`GraphicsWidget`/`QGraphicsScene` are QtWidgets classes that need a real `QApplication` to behave
correctly. Retrofitting `QApplication` into the CLI tool for one kernel is exactly the kind of
`GraphicsWidget`/canvas change that should be verified live rather than assumed to work.

Instead, this extends the already-proven WS12 `--interactive-script` mechanism
(`roadmap_ws12_cli_scripting_mode.md`), running the real `SocNetV` GUI binary with
`QT_QPA_PLATFORM=offscreen` (Qt's standard headless-widget-testing mechanism — no visible window,
same paint/geometry code path as a real session):

- Five new interactive-script commands (`mainwindow.cpp`): `render` (forces a synchronous
  `viewport()->repaint()`, timed), `bulk-node-size <N>` (`slotEditNodeSizeAll()`),
  `bulk-edge-color <name>` (`slotEditEdgeColorAll()`), `move <node> <x> <y>`
  (`Graph::vertexPosSet()`), and `quit` (ends the script cleanly — needed because nothing
  previously stopped a scripted GUI run; every earlier manual test had to be killed externally).
  Each of the first four logs one `qInfo() << "BENCH ..."` line, same pattern as WS14's
  `distances`/`distances centralities` commands. WS12's later uniform-BENCH-logging pass extended
  this to every interactive-script command, not just these four — relevant below.
- Fixed reference script: `scripts/fixtures/render_perf_script.txt` — generates a reference network
  via `erdos-m 2000 40000 undirected` (a new command, `Graph::randomNetErdosCreate()`'s existing
  `G(n,M)` mode exposed to the interactive-script grammar) rather than depending on an external
  dataset file, then runs the fixed sequence: render, bulk-node-size, bulk-edge-color, move,
  render, quit. **Originally used `erdos 2000 0.02 undirected` (G(n,p)) instead** — its own code
  comment already says plainly it's "deterministic-shape (though not deterministic-content, since
  it's still randomized)": node count is exact, but edge count only concentrates around its
  expected value rather than landing on it exactly. `erdos-m`'s `G(n,M)` mode places exactly M
  edges, so N and E are now both exactly reproducible run to run (which specific edges land where
  is still randomized, which doesn't matter for a timing kernel).
- Driver: `scripts/run_render_perf_bench.sh`, structured to match `run_benchmarks.sh`'s existing
  conventions (`scripts/lib/find_socnetv_gui.sh` mirrors `find_socnetv_cli.sh`; same
  auto-detected/overridable baseline-set resolution; same `--record` bootstrapping flow). Runs
  `QT_QPA_PLATFORM=offscreen <gui binary> --interactive-script <fixture>` `RENDER_BENCH_RUNS`
  times (default 7), parses the five `BENCH` lines per run, and compares each metric's
  **median-of-N** `elapsed_ms` against an upper-bound threshold (2× the recorded median-of-N
  reference run) — "must be faster than X ms", not exact equality.
- Baseline: `scripts/perf_baselines/macos-arm64/render_perf_expected.env` and
  `scripts/perf_baselines/macos-m5/render_perf_expected.env`, both recorded from this machine
  (mirrors the compute-benchmark convention of keeping a generic per-arch baseline alongside a
  chip-exact one). `linux-x86_64` doesn't have a recording yet — the script degrades gracefully
  (prints `SKIP` per threshold) when no baseline file exists for the current machine, rather than
  failing; someone running `--record` there once is the way to add coverage for that platform.
  **Not yet verified on Linux** — `QT_QPA_PLATFORM=offscreen` availability/behavior there hasn't
  been checked live.

**Three real bugs found and fixed while re-verifying this kernel, not just a documentation pass:**

1. **The "degrades gracefully" claim above was false until now.** The script's missing-baseline
   check did `exit 2` with a hard `ERROR` when the baseline *file* didn't exist at all — the SKIP
   path only ever covered a file that exists but is missing one specific variable. Concretely: this
   exact recording machine (a MacBook Pro M5) auto-resolves to the generic `macos-arm64` baseline
   set (`uname -m` can't distinguish Apple Silicon generations), which wasn't recorded at the time
   — only `macos-m5` was. So running the script with no explicit override failed outright on the
   very machine it was recorded on. Fixed: the missing-file case now prints an `INFO` notice and
   continues with every metric legitimately `SKIP`ped, matching what this doc always claimed (and
   `macos-arm64` is now recorded too, so the failure mode no longer triggers here anyway).
2. **BENCH-line parsing was silently misaligned.** The comparator greped every line containing
   `"BENCH "` and read the first five as `render, bulk-node-size, bulk-edge-color, move, render`
   by fixed position. That held when this kernel first shipped, but WS12's later uniform-logging
   pass added `BENCH` lines to every command, including the fixture's own setup/teardown
   (`erdos`/`erdos-m`, `delay`, `quit`) — nobody re-tested this script after that landed. The
   comparator was silently reading `erdos-m`'s and `delay`'s timings as `RENDER_INITIAL` and
   `BULK_NODE_SIZE`, while the real `move` and second `render` measurements were dropped entirely
   (only the first five lines were ever read). It still printed "OK" throughout, since the
   mismatched values all happened to be small millisecond timings that stayed under threshold by
   coincidence — this was a real, live correctness gap, not just stale docs. Fixed: the grep now
   allowlists the four metric command names (`^BENCH (render|bulk-node-size|bulk-edge-color|move) `)
   instead of matching any `BENCH` line, which is robust against the fixture gaining more
   setup/teardown commands later without needing this filter touched again.
3. **Single-shot measurement made both the recording and the comparison noisy.** `--record`
   originally ran the fixture exactly once and doubled its raw readings; a repeat-run check found
   `render`'s own timing swinging ~104-131ms (~25%) run to run on identical hardware/build. Two
   baselines recorded minutes apart from the literal same machine (`macos-arm64` vs `macos-m5`,
   before this fix) differed by up to 24% on `BULK_NODE_SIZE` purely from that noise, not from any
   real difference between the two "targets." Fixed: the script now runs the fixture
   `RENDER_BENCH_RUNS` times (default 7, override via env var) and uses the per-metric **median**
   across runs for both recording and comparison — matching `run_benchmarks.sh`'s own
   median-of-N convention for the headless compute kernels, just implemented as an external loop
   here since each run launches a fresh GUI process rather than looping inside one CLI invocation.
   Re-recorded both `macos-arm64` and `macos-m5` from scratch against the corrected parser, the
   new deterministic fixture, and median-of-7: the two are now within a few ms of each other on
   every metric, confirming the previous divergence really was recording noise.

**On-screen vs. offscreen timing — measured, not assumed.** Ran the same fixture script both ways
on the recording machine, 3 times each, to check whether `QT_QPA_PLATFORM=offscreen` changes what's
being measured:

| Command | On-screen | Offscreen |
|---|---|---|
| `render` (initial) | ~297-303ms | ~131-158ms |
| `bulk-node-size` | ~44-49ms | ~46-52ms |
| `bulk-edge-color` | ~45-78ms | ~44-45ms |
| `render` (after mutations) | ~294-303ms | ~120-138ms |

`render`'s actual paint cost is consistently ~2×-2.4× faster offscreen — no compositor/backing-store
round trip on macOS — but the non-paint bulk operations measure the same either way. Confirmed the
`render` command's own timing is genuinely synchronous (not partially deferred) by running two
`render` calls back-to-back with a 2-second gap between them: both reported the identical elapsed
time, which is what you'd expect from `QWidget::repaint()`'s documented synchronous contract (unlike
`update()`, which only schedules a repaint) and wouldn't hold if any part of the cost were being
silently deferred past the call returning. Conclusion: offscreen thresholds are only meaningful
against other offscreen runs, not as a claim about real on-screen user-perceived performance — which
is fine, since this kernel's job is regression detection, not UX measurement.

**Recorded reference run (MacBook Pro M5, 24GB RAM, offscreen, N=2000/~39,800E):**
`render`=154ms, `bulk-node-size`=46ms, `bulk-edge-color`=44ms, `move`=0ms,
`render`-after-mutations=120ms. Thresholds recorded at 2× each (with a 15ms floor for near-instant
operations like `move`, since doubling a 0-1ms reading would produce a threshold too tight to
survive normal jitter).

Why this matters:

`GraphicsWidget` bulk operations (`setEdgeArrowSize`, etc.) currently fire thousands of individual `prepareGeometryChange()` calls without batching.  Phase 1–5 of #240 fix the worst offenders, but without a regression kernel there is no automated guard to prevent the problems returning.

**Not done in this pass, and CI integration stays out too** per WS6.5's "CI is last" policy. The
bigger open gap: the golden harness still has no *correctness* coverage of the canvas, only
performance coverage. Verifying that `GraphicsWidget::setEdgesVisibilityBatch()` actually left the
right set of edges visible/hidden after a relation switch or a unilateral-edge toggle has no
automated check today — the only verification is live manual testing via WS12's
`--interactive-script` mechanism (`roadmap_ws12_cli_scripting_mode.md`), built partly for this
reason. A future canvas kernel should assert on actual `GraphicsWidget` *state* after a fixed
operation sequence (which edges/nodes end up visible, positioned where expected, etc.), not just
timing — same offscreen-rendering constraint as this kernel, but a state-comparison kernel rather
than a timing-threshold one. Could plausibly share this kernel's harness scaffolding, or be a
sibling kernel (`kernel_render_state_v9`-shaped) — not scoped in detail yet.

---

### WS6.7 — Matrix operation golden coverage (`kernel_matrix_v8`) — ✅ Done (v3.7)

`Matrix`-producing operations had no direct golden coverage before this — every existing kernel
only ever checked downstream results (centrality scores, distance values, clique counts), so a
subtle get/set indexing bug in a new storage layout (WS5's A3 target) could still produce correct
downstream numbers by coincidence. `kernel_matrix_v8` dumps raw matrix contents instead, for all
seven categories audited as having no direct coverage: adjacency (`AM`), inverse (`invAM`),
distances (`DM`), similarity (a local `SCM`, matching metric), reachability (`XRM`), walks (`XM`,
fixed length) and total walks (`XSM`), and clique co-membership (`CLQM`).

**New `Graph` accessors** (`matrixAdjacency()`, `matrixAdjacencyInverse()`, `matrixDistances()`,
`matrixReachability()`, `matrixWalks()`, `matrixTotalWalks()`, `matrixCliqueCoMembership()`,
`graph.h`) expose these previously-private fields read-only-by-convention — non-const because
`Matrix::item()`/`rows()`/`cols()` are themselves non-const everywhere in the codebase.

**Dump format is size-dependent**, decided empirically:
- Small fixtures (`TinyPath_N3_E2`, `TinyDisconnected_Undir_N6_E4` — reused, no new fixtures added)
  dump the full N×N grid.
- The big/sparse tier uses `Benchmark_BA_Directed_N500_m3` (N=500, already shipped, already used by
  `run_benchmarks.sh`) rather than an out-of-repo `~/socnetv/library` path, so the baseline
  reproduces on any machine or in CI. It dumps a compact summary instead of the full grid (250K
  cells): per-row/column sums, the trace, and five sampled cells (corners + center).
- **Total walks (`XSM`) is skipped above N=50** (`kTotalWalksSkipThreshold`,
  `kernel_matrix_v8.cpp`) — measured directly, not assumed: summing matrix powers up to N-1 took
  **~9.2 minutes (553,940 ms)** at N=500 on the big fixture (MacBook Pro M5, unoptimized build).
  Clique co-membership needed no such gate — **6 ms** on the same run, since Bron-Kerbosch stays
  cheap on sparse graphs — so it's computed unconditionally at every size.

**Verified the actual point of this kernel**: manually introduced a one-line off-by-one into
`Matrix::item()` (`(c + 1) % m_cols` instead of `c`), reran `run_golden_compares.sh` — all three
matrix baselines went red, plus several pre-existing kernels that also read `Matrix::item()`
indirectly (distance, prominence, reachability, walks). Reverted; suite is clean again. WS5's A3
now has the safety net it was waiting on.

---

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

### WS6.4 — Tighten determinism, measurement stability, and reporting

Goal:

Reduce false positives and reduce noise sensitivity, especially in IO benchmarks.

Possible improvements:

- clarify how BUILD_TYPE / configuration is detected and printed by scripts
- use median-of-N consistently where useful (especially IO load tests)
- separate IO load-time thresholds from compute-time thresholds where needed
- document baseline update rules (rare; only for real semantic fixes)

#### Known: parallel reduction is FP non-deterministic, with ~2.9× tolerance headroom

Found while measuring something else. **Not a live failure — a thin margin worth knowing
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

## Work Rules

- Keep outputs stable (version schemas when changing format).
- Baseline regeneration should be treated as exceptional.
- Any "FAIL" in benchmarks must be investigated; if it is noise, prefer mitigation via more stable measurement rather than loosening thresholds by default.
- WS6 work should remain incremental: small changes, deterministic evidence, and consistent scripts.

## Open Findings

### `run_benchmarks.sh` reports `BUILD_TYPE=Debug` even against a Release binary

Script-reporting detail, not a functional bug, but it can confuse future contributors reading
benchmark output. Good tiny WS6 task whenever there's a lull.

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

## Resolved Findings

### Shipped-dataset roundtrip script fails on a custom-delimiter fixture (#256) ✅ Fixed

`run_io_roundtrip_shipped_datasets.sh` globs every file under `src/data` and loads each one via
the CLI using its default delimiter (`" "`, set in `socnetv_cli.cpp`'s `delimOpt`), with no
per-file override table. `src/data/TinyAdj_Dir_N3_E4_clucof.adj` used `|` as its column delimiter
— likely added alongside the directed clustering-coefficient fix (#58, `81e82a46`) to exercise the
GUI's Import-dialog custom-delimiter option — so it failed under the script's blanket
space-delimiter assumption. This went unnoticed because the script isn't part of the required
regression gate (see the three scripts listed under "Regression discipline" in
`README_DEVELOPER_NOTES.md`) or CI.

**Fixed (`802b0097`)** by reformatting the fixture file itself to plain space
delimiters, matching every other `.adj` file in `src/data/` — simpler than either of the two
alternatives originally considered here (a per-file delimiter override table, or promoting the
script into CI), since the nonstandard delimiter wasn't actually load-bearing for what the fixture
tests (directed clustering coefficient). Promoting this script into the required gate / CI is still
open, now unblocked.

### Perf benchmark baselines predated the M1 DistanceEngine speedup — ✅ Fixed on all three sets

All three committed baselines predated M1's DistanceEngine parallelization
(`7900809e`/`11da8ef4`, the same day v3.6 shipped) by months:
`scripts/perf_baselines/macos-arm64/perf_expected.env` (`ada8e613`),
`scripts/perf_baselines/macos-m5/perf_expected.env` (`5661b7eb`), and
`scripts/perf_baselines/linux-x86_64/perf_expected.env` (`013c05ce`). `run_benchmarks.sh`
still ran and reported "OK" against these stale baselines, but the "beats baseline by 30–70%"
results on the DistanceEngine-heavy benchmarks (EIES48, BA500, DIST_GRAPHML) weren't drift or
noise — they were exactly M1's real 2.7×–8.3× speedup being measured against a pre-M1 floor. A
regression eating into half of M1's gain would still have silently passed as "faster than
baseline." Found when a benchmark run was (wrongly) cited as regression evidence for a small WS3
M2 change; the actual evidence for that change was architectural (equivalent lookup complexity),
not the benchmark comparison, which prompted checking why the margins looked so large.

**`macos-arm64` and `macos-m5` fixed (`9679782f`)**, both from this machine: checked out the `v3.6`
tag directly (a clean release point, not just "whatever HEAD happens to be today"), built, and ran
`run_benchmarks.sh --record` there — `macos-arm64` via the script's own `uname`-based
auto-detection (which can only ever resolve to `macos-arm64` on any Apple Silicon Mac, M1 through
M5 — `uname -m` doesn't report chip generation), `macos-m5` via an explicit
`BENCH_BASELINE_SET=macos-m5 BENCH_RECORD_ALLOW_OVERRIDE=1` override for this specific chip.
Verified against current `develop`: every benchmark on both sets now lands within 1–5% of its
baseline (not 30–70% "faster"), confirming no regression from the WS3/WS10 work landed since v3.6,
and that future comparisons on this machine are meaningful again.

**`linux-x86_64` fixed (`b9a88096`)**, from a 12-core Ryzen Linux x86_64 box: same method — a
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
