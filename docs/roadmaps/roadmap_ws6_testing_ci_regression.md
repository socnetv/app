# Testing / CI / Regression Roadmap (WS6)

## Goal

Prevent silent regressions during modernization.

## Status

🚧 Ongoing, supporting workstream (no fixed end state — continuously active underneath every other
workstream). CI integration (WS6.5) is explicitly last and not started.

## Background

### What `socnetv-cli` is, and why it exists

SocNetV has no unit-test framework. Instead, correctness is checked by running the actual
computation code (the same `Graph`/`DistanceEngine`/algorithm-slice code the GUI calls) against a
real network file, capturing the numeric result as JSON, and comparing that JSON against a
committed "known good" copy on every future run. If a refactor accidentally changes a result, the
comparison fails immediately — this is what "golden testing" / "regression testing" means in this
codebase.

`socnetv-cli` is the tool that runs those computations headlessly — no window, no `QApplication`,
just `QCoreApplication` plus whatever `Graph` machinery a given computation needs. It's built
alongside the main app (`cmake ... -DBUILD_CLI=ON`) as a separate small binary. Concretely:

```bash
./build/socnetv-cli -i src/data/Padgett.graphml -f graphml --kernel clustering \
  --compare-json src/tools/baselines/clustering/padgett_clustering.json
```

This loads `Padgett.graphml`, runs the `clustering` kernel's computation, and compares the result
against the committed baseline file — exiting non-zero (and printing what differs) if anything
doesn't match.

**A kernel** is one self-contained computation family exposed via `--kernel <name>` — `distance`,
`clustering`, `matrix`, `signed`, etc. Each kernel owns its own JSON schema (`schema_version`,
versioned independently per kernel) and its own compare logic; kernels don't share a schema, since
they protect entirely different parts of the codebase (BC/CC vs. raw matrix contents vs.
connectivity component counts, etc.). The full kernel list, CLI flags, and exact JSON schemas live
in [`docs/SOCNETV_CLI_REGRESSION_TOOL.md`](../SOCNETV_CLI_REGRESSION_TOOL.md) — not duplicated
here; this file is about the *testing strategy* (what to cover, how it's organized, what's still
missing), that one is the *reference* (what each kernel's flags/output actually are).

**A baseline** is the committed "known good" JSON file a kernel run is compared against — one file
per (dataset, kernel, flag combination), stored under `src/tools/baselines/<kernel>/`. A baseline
is only as trustworthy as how it was produced: dumping one from the app's *current* output only
proves *self-consistency* (today's output matches itself later) — it says nothing about whether
that output was ever *correct*. WS6.8 (below) exists specifically to close that gap: a baseline
should ideally be checked, at least once, against a result computed independently of SocNetV
entirely (by hand, or a short standalone script), not just accepted as "whatever the app printed."

Both GUI and CLI load graphs through the same IO mutation pipeline introduced in WS4
(`Parser` → `IGraphParseSink` → `Graph`), via `tools/headless_graph_loader.h`, which blocks on
`Graph::signalGraphLoaded` (falling back to `Parser::finished`). This is what makes CLI kernel
output valid regression evidence for GUI-triggered behavior, rather than a separate code path
being tested in isolation.

> **Before committing any change described in this file:** run
> `./scripts/run_golden_compares.sh`. All golden JSON baselines must still pass.

---

## What WS6 Delivered

### Regression scripts

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
- IO roundtrip baselines are committed in-repo under `src/tools/baselines/io_roundtrip/`.

Perf baselines (`scripts/perf_baselines/<platform>/perf_expected.env`) are re-recorded from a clean
tagged release, not arbitrary `develop` HEAD — a stale pre-M1-speedup baseline once made every
benchmark misleadingly report "30-70% faster than baseline" regardless of what was actually being
tested (same failure mode recurred with WS15 P4's parallelization work, discovered when
`develop`'s benchmarks kept reporting large "faster than baseline" numbers against baselines that
predated all of WS15 P4). Current baseline provenance: macOS `arm64`/`m5` (compute + render, all
four `.env` files) re-recorded from `v3.7` (2026-09-15); Linux `x86_64` (compute only, no render
baseline exists for that platform) recorded from `v3.7` at the time of its own commit (2026-08-27),
still current.

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
sibling kernel (`kernel_render_state_v9`-shaped) — not scoped in detail yet. **This gap is tracked
as open work below, under WS6.1.**

### WS6.7 — Matrix operation golden coverage (`kernel_matrix_v8`) ✅ Done (v3.7)

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

**Extended for #279 (v3.8)**: the `similarity` category originally only ever ran Simple Matching
(hardcoded `METRIC_SIMPLE_MATCHING`), so `similarityMatrix()`'s Jaccard path and
`pearsonCorrelationCoefficients()` (a separate `Graph` method, never called by this kernel at
all) had zero golden coverage — exactly the gap that let #279's divide-by-zero/NaN bug on both
go unnoticed. Added a `--similarity-measure simple_matching|jaccard|pearson` CLI flag
(`cli_common.h`, `socnetv_cli.cpp`) so the kernel can select and dump each one; `pearson` routes
to `createMatrixSimilarityPearson()` instead of `createMatrixSimilarityMatching()`. Three new
baselines on `TinyArc_Dir_N2_E1` (N=2, one directed arc) pin the fix: with the default
`diagonal=false`, comparing the network's only pair excludes every sampled column, driving
Jaccard/Simple-Matching's `ties` and Pearson's effective sample size to exactly zero — verified
directly (not assumed) by building the pre-#279-fix commit in a scratch worktree and confirming
`nan` in the dumped JSON there vs. `0` post-fix on the same fixture/measure.

**Extended again for the Jaccard `RAND_MAX` fix (v3.8)**: a follow-on audit of `matrix.cpp`
(same pass that produced #279/#280) found `similarityMatrix()`'s Jaccard branch didn't exclude
`RAND_MAX` (the "unreachable" sentinel) from its match/ties count the way `distancesMatrix()`
does — invisible when similarity runs on the adjacency matrix (`AM`, never contains
`RAND_MAX`), only reachable when it runs on the geodesic distances matrix (`DM`,
`graph_reports.cpp`'s `createMatrixSimilarityMatching(DM, ...)` path) on a disconnected
network. Added a `--similarity-input adjacency|distances` CLI flag (mirroring
`--similarity-measure`) so the kernel can select which matrix feeds the `similarity` category;
`DM` is already computed unconditionally earlier in the kernel, so no extra work is needed to
supply it. One new baseline on `TinyDisconnected_Undir_N6_E4` (already used above as the
full-grid small fixture) with `--similarity-measure jaccard --similarity-input distances`
pins the fix. Verified live impact
before fixing (not assumed): cell (D,F) read `0.75` pre-fix (a false-positive similarity from
three shared-unreachable sample columns) vs. `0.0` post-fix, confirmed via a scratch worktree
comparison the same way #279 was.

Three smaller, currently-unreachable `Matrix` issues found during the same audit were hardened
alongside the Jaccard fix, with no dedicated golden coverage (no real caller reaches either
path today, so there's no natural kernel entry point without inventing a synthetic harness
disproportionate to the fix):
- `product(A, B, symmetry=true)` wrote out of bounds if `A.rows() != B.cols()` — now guarded
  with an early return, matching the existing `A.cols() != B.rows()` incompatibility check in
  the same function.
- `productByVector(..., leftMultiply=true)` computed the wrong output length and read past
  `cols()` for non-square input — fixed to match the loop bounds its own doc comment already
  specified (`out` has `cols()` elements, `in` has `rows()`, for the left-multiply case).
- `distancesMatrix()`/`similarityMatrix()`/`pearsonCorrelationCoefficients()` all assume a
  square input (`N` taken from `rows()` for every axis, in every `varLocation` mode) — true for
  every current caller (`AM`/`DM`, always square), now documented explicitly via `@note` rather
  than left implicit.

### `kernel_connectivity_v7` — weak/strong connected components (#85, #272) ✅ Done

- `Graph::graphWeaklyConnectedComponents()` — BFS treating all edges as undirected (weak connectivity); caches count in `m_graphWeaklyConnectedComponents` and per-node IDs in `m_vertexComponentId`. Cache invalidated with `resetDistanceCentralityCacheFlags()`.
- `Graph::graphStronglyConnectedComponents()` (#272) — Tarjan's SCC algorithm (single DFS pass,
  no graph transpose needed), respecting edge direction; caches count in
  `m_graphStronglyConnectedComponents`, same invalidation sites as the weak variant. Reports a
  count only, not per-vertex SCC membership (`m_vertexComponentId` stays weak-only — see the
  function's own doc comment in `graph_distance_facade.cpp`).
- CLI: `--kernel connectivity --connectivity-type weak|strong` (default `weak`; ignored on
  undirected graphs, where the two notions coincide).
- Six baselines committed: `TinyDisconnected_Undir_N6_E4` (3 components), `TinyDisconnected_Dir_N5_E3`
  (2 weak / 5 strong components), `TinyPath_N3_E2` (1 component / connected), plus `__STRONG`
  variants for `TinyDisconnected_Dir_N5_E3`, `TinyArc_Dir_N2_E1` (1 weak / 2 strong), and
  `TinyWeaklyConn_Dir_N3_E2` (1 weak / 3 strong).
- **Connectivity semantics table** (what the kernel computes and what the UI reports):

  | Graph type | Topology | Mode | Components | `connected` | UI message |
  |---|---|---|---|---|---|
  | Undirected | All nodes reachable | n/a | 1 | true | "connected (1 component)" |
  | Undirected | N isolated islands | n/a | >1 | false | "disconnected (N components)" |
  | Directed | Every pair has a directed path | weak | 1 | true | "weakly connected (1 component)" |
  | Directed | Every pair has a directed path | strong | 1 | true | "strongly connected (1 component)" |
  | Directed | A→B only (not B→A) | weak | 1 | true | "weakly connected (1 component)" — one island |
  | Directed | A→B only (not B→A) | strong | 2 | false | "not strongly connected (2 components)" |
  | Directed | Two separate islands | weak | >1 | false | "disconnected (N weakly connected components)" |

  **Design rationale:** the GUI's Connectedness action (`slotAnalyzeConnectedness()`) asks the
  user weak vs. strong for any directed graph, rather than silently picking one — see #272. The
  old approach of treating `isConnected()`/SSSP as the de facto strong-connectivity source of
  truth is gone; `graphStronglyConnectedComponents()` is now the single, explicit, dedicated
  method for that question.

### `kernel_vertex_connectivity_v9` — local/global vertex connectivity (#7, WS11) ✅ Done

- `Graph::graphNodeConnectivity(source, target, respectDirection)` — local kappa(s,t), via
  vertex-split max-flow (Edmonds-Karp). Returns a `NodeConnectivityResult{status, value}`, not a
  plain int: `Adjacent` (s,t directly connected — no finite cut exists) and `Invalid`
  (nonexistent/equal vertices) are distinct outcomes from `Ok` on purpose, precisely to avoid
  repeating #271's class of bug (a sentinel int silently misused as a bool/count).
- `Graph::graphConnectivity(respectDirection)` — global kappa(G), the naive pairwise-minimum
  over all non-adjacent pairs, pruned by the minimum-degree bound (Whitney's inequality) and an
  early exit at 0. Deliberately not the smarter O(n) algorithm (Even 1975) - see the function's
  own doc comment for why.
- CLI: `--kernel vertex_connectivity --conn-mode local|global [--conn-source S --conn-target T]
  --connectivity-type weak|strong`.
- Seven baselines committed, deliberately Tiny*/toy datasets only (global mode is O(n^2) local-
  connectivity computations in the worst case - not something to run against the 500-node
  `Benchmark_*` datasets used elsewhere in this file): `TinyPath_N3_E2` global (kappa=1) and
  local, both a non-adjacent pair (1,3 → ok/1) and an adjacent one (1,2 → `Adjacent`, no value);
  `TinyDisconnected_Undir_N6_E4` global (kappa=0); `TinyWeaklyConn_Dir_N3_E2` global in both weak
  (kappa=1) and strong (kappa=0, not strongly connected) mode; `TinyComplete_Undir_N4_E6` (new
  dataset, K4) global (kappa=3=n-1, confirming the complete-graph case needs no special code path
  - see the function's doc comment).
- Also verified against two independent, non-baseline checks: the Petersen graph
  (`src/data/Petersen_Graph.paj`) returns kappa(G)=3, the textbook value for that well-known
  3-regular, 3-connected graph; and live in the GUI (Analyze > Cohesion > Node/Graph
  Connectivity), which reproduces the same CLI-verified numbers end-to-end.

---

## What Remains Open

### WS6.1 — Expand CLI kernel coverage (UI-adjacent functionality, headless)

Goal:

Expose more “UI-visible” functionality through deterministic CLI kernels, so it becomes testable headlessly without the GUI.

Examples of high-value additions:

- random network generators (deterministic via fixed seeds)
- layout / visualization computations runnable headlessly (compute-only; no QtWidgets/QtCharts)
- additional analysis workflows that users typically trigger from UI
- a canvas *correctness* (not just performance) kernel — see WS6.6's "Not done in this pass" note
  above: no automated check exists today that `GraphicsWidget` state (visible/hidden edges, node
  positions) after a fixed operation sequence matches expectations, only manual
  `--interactive-script` testing. Could share WS6.6's harness scaffolding or be a sibling kernel.
- **kernel_signed_v10** (not yet built, in progress — WS18 P2) — signed-network analysis outputs,
  starting from `DistanceEngine::computePotentials()` (Johnson's-algorithm reweighting pass, added
  but not yet wired into `dijkstraSSSP()`/`runAllSources()`):
  - New `Graph::graphComputePotentials(inverseWeights, outPotentials)` — constructs a
    `DistanceEngine`, calls the already-private `computePotentials()`, copies out the per-vertex
    potential vector, returns whether a negative cycle was detected. `Graph` is already a friend of
    `DistanceEngine`; no new friendship needed. Kept as a standalone probe, not part of the
    refuse-and-compute pipeline `graphDistancesGeodesic()` drives.
  - `src/tools/cli/kernels/kernel_signed_v10.{h,cpp}`, `--kernel signed`: dumps per-vertex
    potentials and `negative_cycle_detected`, same `dataset`/`counts`/`graph` boilerplate every
    other kernel emits. Registered in `socnetv_cli.cpp` and `CMakeLists.txt`.
  - Baselines: `WeightedTies_Dir_N5_SigmaRegression` (already has an independently hand-verified
    ground truth from #283) plus a new small fixture with a genuine negative cycle.
  - **Designed to grow, not be replaced**, as WS18's later phases land: P3 (PN centrality) and P4
    (structural balance ratio) add new JSON sections to this same kernel rather than spawning
    `kernel_pn_v11`/`kernel_balance_v12` — all outputs describe the same "signed-network analysis
    of this dataset" concept, on the same dataset shape (fixtures with signed edge weights).
- **kernel_attribute_import_v7** (not yet built) — CSV/JSON attribute import + export roundtrip (#227, #232):
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

More of the application's "user-facing" features become regression-testable without the UI.

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
chain length grow, and WS6.8 explicitly plans to add *larger* datasets. A `geom.net`-scale baseline
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
spread at `geom.net` scale (7343 nodes) to confirm the new margin holds where WS6.8 is heading.

Reproduce: run the same `--dump-json` invocation N times and hash the output with `_ms` fields
stripped.

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

- reformat output as a human-readable summary table (case, kernel, pass/fail, timing) instead of
  the current flat `==> filename` / raw CLI output / `OK`/`FAIL` stream - the detailed streaming
  output is fine to keep for diagnosing one failing case, but the default end-of-run view should
  be scannable at a glance, especially as the baseline count keeps growing.

Outcome:

Faster local workflows and easier diagnosis when one suite fails.

### WS6.5 — CI integration (LAST)

Goal:

Once local harness coverage is broad and stable, add CI checks to prevent regressions from landing silently.

Policy:

CI must not become the primary place where developers discover breakage.

CI should run a carefully chosen subset by default:

- build (Release preferred)
- a "fast" golden subset
- optionally a "fast" benchmark subset

Heavier suites can run nightly or on-demand.

### WS6.8 — Independently verify every algorithm's correctness, not just its self-consistency

**Status: active priority (2026-09-22)** — originally queued during the Katz/Bonacich work
(2026-08), deferred past v3.7. Two more real bugs found the same way since (#283, #286, both
2026-09) make this no longer a someday-audit: it's the standing bar every algorithm needs to clear.
**Absorbs the former WS6.2** ("systematically expand datasets and coverage") — that section's goal
(more networks/edge cases, tested in a structured way) was a looser, less concrete restatement of
what this section now demands precisely; keeping both risked them drifting apart or contradicting
each other. Its dataset-breadth concerns (parser edge cases per format, small/medium/large scale)
are folded into the Approach below as a distinct dimension from independent-verification coverage,
not dropped.

Goal:

Golden baselines catch *regressions* (today's output differs from yesterday's), but say nothing
about whether the *original* baseline was ever mathematically correct. Every baseline currently in
`src/tools/baselines/` was accepted once, at dump time, without independent verification against a
hand-computable ground truth. **Every kernel/algorithm family should have at least one baseline
independently verified against ground truth computed outside SocNetV** (by hand, or a short
standalone script — never by re-deriving the expected value from SocNetV's own code) — not just a
risk-based sample. A risk-based order (below) still decides which cases get done first; it no
longer decides whether a case gets done at all.

Motivating precedent — three separate incidents now, same failure shape each time: a bug baked
into the very first implementation, invisible for years because every later golden run only ever
checked self-consistency (today's output vs. yesterday's), never against an outside ground truth:

- The `Matrix::powerIteration()` divide-by-zero substitution (`norm = 1`) leaking into the reported
  `lambdaMax` on nilpotent (directed, cycle-free) matrices (2026-08, Katz/Bonacich work).
- A stale-cache bug where a rejected alpha/beta silently short-circuited every later recompute
  (2026-08, same work) — invisible to golden compares since each baseline is dumped once, not
  re-run with changing inputs.
- `dijkstraSSSP()`'s tie-breaking bookkeeping not fully updated on a strictly-improved relaxation
  (#283, 2026-09) — inflated BC/SBC on any weighted network where a vertex is relaxed more than
  once. Dated to the original 2014 Dijkstra implementation.
- Diameter tracked as a running max over relaxation *events* instead of each vertex's *final*
  distance (#286, 2026-09) — same 2014 origin commit as #283, same "vertex relaxed more than once"
  precondition, found only because a WS18 GUI-testing session happened to hand-verify a diameter
  value and it didn't match.

All four were found only by hand-deriving or independently scripting the expected value against a
small, deliberately-constructed test network, then comparing against what the app actually
produced — never by trusting the existing baseline, and never by re-deriving the expected value
from SocNetV's own algorithm (that would just re-confirm the same bug, not catch it).

Approach:

- Prioritize edge cases most likely to hide latent bugs, per the pattern above: directed +
  nilpotent/cyclic structure, isolates, self-loops, zero-weight edges, disconnected components,
  weighted + inverted-weight combinations, any network where some vertex is relaxed more than once
  during Dijkstra (the exact precondition #283/#286 both needed), and any kernel that recently
  changed (`vertex_connectivity`, `connectivity`, `matrix`, `signed` are the newest families and
  haven't had this treatment at all yet).
- For each kernel, hand-derive the expected result independently (small enough networks that this
  is tractable by hand or a short verification script) and compare against the current baseline —
  not just against the app's current output, since the app could be self-consistently wrong.
- **A single hand-derivation attempt is not automatically trustworthy — use a second independent
  method when the first result is at all surprising.** Found live during the `matrix` weighted
  baseline work (2026-09-23): a first hand-derivation of `similarity`'s simple-matching values
  guessed the wrong column-exclusion rule (assumed "skip the diagonal cell", the actual rule is
  "skip column j whenever j equals either actor being compared") and produced plausible-looking but
  wrong expected values. A second, independent method — a short standalone script implementing the
  measure from its algorithmic definition (not copied from SocNetV's C++), separate from external
  tools used elsewhere for cross-checking (see this repo's own conventions for that) — caught the
  mismatch and confirmed the app's output, not the first hand-derivation, was correct. The lesson:
  "independently verified" means the verification method must itself be checked, not just run once
  and trusted because it produced *a* number.
- **Prefer cross-checking against established outside scripting tools over hand derivation when
  the case is tractable that way** — a standalone script re-implementing an algorithm from its
  published definition remains the fallback for cases those tools don't cover natively (schema
  details specific to this app, e.g. `links_sna` vs `ties_graph` counts) or where using one would
  be disproportionate. This matters for one non-obvious reason found live during the `clustering`
  work (2026-09-23): a metric can have more than one legitimate published definition — SocNetV's
  directed local clustering coefficient (union of in/out neighbourhood, ordered-pair denominator)
  and a well-known outside library's default directed clustering coefficient (Fagiolo 2007,
  geometric-triangle based) are **both correct, but different metrics**, and comparing against the
  wrong one produces a false mismatch. When an outside-tool cross-check disagrees, check whether the
  two sides are actually computing the same defined quantity before concluding either is wrong —
  read SocNetV's own doc comment for the formula it claims to implement, then independently
  reimplement *that specific formula* (not the outside tool's default) as the real check.
- Where a baseline is found to be wrong, follow the same discipline used for the `powerIteration`
  fix and #283/#286: confirm the fix is unambiguously correct, understand exactly what changes and
  why, get explicit sign-off before touching previously-"passing" baselines, then re-dump with a
  clear commit explaining what was wrong and how it was verified.
- Given the scope (9 kernel families, ~78 baseline files as of 2026-08, growing), this is
  incremental work threaded through ordinary development, not a single dedicated pass — every new
  algorithm/measure added from now on gets independent verification as part of landing it (see
  WS6's own Work Rules below), and existing kernels get worked through opportunistically in the
  risk-based order above.
- **Dataset/format breadth** (former WS6.2), a distinct dimension from independent-verification
  coverage above but pursued alongside it: grow the dataset suite gradually; include representative
  small/medium/large graphs; include tricky parser edge cases per format (GraphML/DOT/Pajek
  quirks); where formats lack exporters, keep using export-skipped baseline locking; prefer shipped
  datasets under `src/data` where possible, add external datasets only if licensing permits.

#### Coverage matrix (living checklist — update as work lands)

**First finding, worth internalizing before adding fixtures anywhere below: not every kernel has
a meaningful weighted axis.** Several families operate purely on edge *existence*, never read a
weight value, so a "weighted vs unweighted" baseline pair for them would be identical output by
construction — a wasted baseline, not real coverage. Checked by reading the actual algorithm
source (not assumed):

| Kernel | Reads edge weight values? | Where checked |
|---|---|---|
| `connectivity` | No — `graphWeaklyConnectedComponents()`/`graphStronglyConnectedComponents()` are pure BFS/DFS over edge presence | `graph_distance_facade.cpp` |
| `vertex_connectivity` | No — Menger's-theorem max-flow via vertex-split, no edge capacities used | `graph_connectivity.cpp` |
| `reachability` | No — `apspDistance() != RAND_MAX`, path existence only | `kernel_reachability_v2.cpp` |
| `walks` | No — `walksBetween()` is adjacency-matrix power, binary | `kernel_walks_v3.cpp` |
| `matrix` | **Yes** — `createMatrixAdjacency(considerWeights=true)` stores real weight values in `AM`/`DM`/similarity | `graph_matrix_adjacency.cpp` |
| `clustering` | No — `clusteringCoefficientLocal()` only checks edge *presence* to build the neighbourhood; `considerWeights` merely gates a zero-weight-edge exclusion filter already active regardless, so a weighted vs. unweighted run only differs when the fixture has an edge that becomes weight-zero — none of the 8 existing fixtures do. Confirmed 2026-09-23: `DunbarGelada_H22a` and `StokmanZiegler_Netherlands`'s `W0`/`W1` baseline pairs are byte-identical apart from the `run`/timing fields. | `graph_clustering_coefficients.cpp` |
| `prominence`, `distance`, `signed` | Yes | (established, weighted baselines already exist) |

So for `connectivity`/`vertex_connectivity`/`reachability`/`walks`/`clustering`, the
directed/undirected axis is the only topology axis that matters — a weighted variant is not
missing coverage, it's a no-op. Don't add one.

**Status per family, as of 2026-09-23** (Dir/Undir = at least one baseline of each topology exists;
Weighted col is N/A where the table above says the kernel doesn't read weights):

| Kernel | Dir+Undir? | Weighted axis | Independently verified? |
|---|---|---|---|
| `connectivity` | ✅ yes | N/A (topology-only) | ✅ done 2026-09-23 — all 10 baselines hand-derived from `.paj` source and matched exactly (component counts, weak vs. strong) |
| `matrix` | ✅ yes | ✅ done 2026-09-23 | ✅ done 2026-09-23 — `adjacency`/`distances`/`reachability`/`clique_comembership` on `TinyPath_N3_E2` hand-verified; `Benchmark_BA_Directed_N500_m3`'s adjacency (row/col sums, trace, 5 sampled cells) cross-checked via independent `.paj` parse, all match; `similarity` already verified for #279/#280. New weighted baseline `TinyDirWeighted_N3` (A→B:2, B→C:3, directed, one-way only) added and independently verified two ways: hand-derived adjacency/distances/reachability, and a from-scratch Python reimplementation of simple-matching similarity's algorithm (not copied from SocNetV source) cross-checked against the dumped `similarity` category. Also confirms `clique_comembership` correctly requires **reciprocal** ties for adjacency (`reciprocalNeighborhoodList()`) — a one-directional arc doesn't count, so this fixture's expected result is the identity matrix (no shared cliques), not what a naive undirected-adjacency assumption would predict. |
| `vertex_connectivity` | ✅ yes | N/A (topology-only) | ✅ done (pre-existing) — Petersen graph kappa(G)=3 textbook cross-check + live GUI cross-check |
| `reachability` | ✅ yes (1 dir + 1 undir) | N/A (topology-only) | ✅ done 2026-09-23 — both baselines cross-checked cell-by-cell against a standalone Python BFS reimplementation (not SocNetV code). `DunbarGelada_H22a` (undirected, N=12): fully connected, all 144 pairs reachable, density 1.0, every matrix cell independently confirmed 1. `StokmanZiegler_Netherlands` (directed per DL FULLMATRIX loading, though the underlying matrix happens to be symmetric; N=16, node 16/NSU fully isolated): 226/256 reachable pairs, density 0.8828125, full 16×16 matrix matched cell-by-cell. |
| `walks` | ✅ yes (1 dir + 1 undir + 1 tiny) | N/A (topology-only) | ✅ done 2026-09-23 — all 3 baselines cross-checked cell-by-cell against a standalone Python adjacency-matrix-power reimplementation (not SocNetV code). `TinyPath_N3_E2` (K=2, undirected N=3): total 6, matrix matched. `DunbarGelada_H22a` (K=6, undirected N=12): total 136644, all 144 cells matched. `StokmanZiegler_Netherlands` (K=6, directed load path over a symmetric binarized matrix, N=16): total 5129002, all 256 cells matched. |
| `clustering` | ✅ yes | N/A (topology-only, see finding above) | ✅ done 2026-09-23 — all 8 baselines cross-checked against an outside-tool reimplementation. `TinyPath_N3_E2`, `TinyDirChain_N3`, `Krackhardt_Kite_N10`, and `DunbarGelada_H22a`'s undirected CLC/cliques/triad-census all matched a standalone networkx script directly. `Sampson_Monks_N18` (mixed `*Arcs`+`*Edges`, directed) and both `StokmanZiegler_Netherlands` variants matched on cliques and triad census, but directed CLC first came back *mismatched* against `nx.clustering()` — turned out to be two different legitimate formulas, not a bug: SocNetV uses union-of-in/out-neighbourhood with an ordered-pair denominator `k*(k-1)` (documented directly in `clusteringCoefficientLocal()`), while networkx's directed CLC defaults to the Fagiolo (2007) geometric-triangle formula. Reimplementing SocNetV's own documented formula from scratch (independently, not copied from its C++) matched the baseline exactly, confirming the baseline itself is correct for what it claims to measure. Weighted variants (`DunbarGelada_H22a`, `StokmanZiegler_Netherlands`) reconfirmed byte-identical to their unweighted counterparts, consistent with the no-weighted-axis finding above. |
| `distance` | partial — weighted cases skew directed | ✅ yes | ✅ partial — several verified during #283/#286 |
| `prominence` | mostly directed | ✅ yes | ⚠️ partial — spot-checked during Katz/Bonacich work, not exhaustive |
| `signed` | directed only (signed implies weighted; undirected signed ties are not a modeled case) | N/A (weight required by definition) | ✅ done — this session's WS18 work (hand + independent script) |
| `io_roundtrip` | per-format | per-format | N/A — fidelity check, not a numeric algorithm |

**Next actions implied by this table**, in the risk-based order from Approach above:
1. ~~Add the missing weighted `matrix` baseline~~ — done 2026-09-23 (`TinyDirWeighted_N3`).
2. ~~Independently verify `reachability`'s 2 existing baselines~~ — done 2026-09-23.
3. ~~Independently verify `walks`'s 3 existing baselines~~ — done 2026-09-23.
4. ~~Independently verify `clustering`'s 8 baselines~~ — done 2026-09-23.
5. Extend `distance`/`prominence` verification to full breadth (currently partial).

Rules:

- add datasets incrementally; baseline additions must be reviewed (do not bulk-regenerate)

### Open findings

#### Windows/MSVC build warnings: C4458 shadowing, C4996 Qt6 deprecations

**Status: deferred until after the v3.7 release** — noticed during the v3.7 `Release SocNetV`
workflow run (2026-08), Windows leg takes noticeably longer than macOS/Linux and is noisy with
warnings not seen on the other two platforms.

- `C4458` ("declaration of 'x' hides class member") — worth checking whether any of these are in
  the same family as this session's `GraphVertex` uninitialized-member findings (#274): shadowed
  member names are exactly the kind of thing that makes "did this actually set the member, or a
  local shadowing it?" hard to eyeball. Could be entirely benign constructor-parameter shadowing
  too - needs an actual look at the specific sites, not assumed either way.
- `C4996` ("`QCheckBox::stateChanged` is deprecated: Use `checkStateChanged()` instead") - plain
  Qt6 API migration debt, unrelated to the above.

Not investigated further yet - just captured here so it isn't lost. Pick up post-3.7.

#### `run_benchmarks.sh` reports `BUILD_TYPE=Debug` even against a Release binary

Script-reporting detail, not a functional bug, but it can confuse future contributors reading
benchmark output. Good tiny WS6 task whenever there's a lull.

#### Continuous release page shows a stale "published" date (#255 follow-up)

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

---

## Work Rules

- Keep outputs stable (version schemas when changing format).
- Baseline regeneration should be treated as exceptional.
- Any "FAIL" in benchmarks must be investigated; if it is noise, prefer mitigation via more stable measurement rather than loosening thresholds by default.
- WS6 work should remain incremental: small changes, deterministic evidence, and consistent scripts.
- **New algorithm/measure work must independently verify at least one result before its first
  golden baseline is trusted** (see WS6.8) — a fixture whose expected value was hand-derived or
  computed by a standalone script outside SocNetV, not just "the app agrees with itself." A
  self-consistent golden baseline proves nothing about correctness on its own; three separate
  bugs (`powerIteration`, #283, #286) shipped silently for years hiding behind exactly that gap.
