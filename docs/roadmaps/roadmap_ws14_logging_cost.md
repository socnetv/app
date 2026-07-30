# Logging Cost & Release-Build Hygiene Roadmap (WS14)

Tracking issue: [#268](https://github.com/socnetv/app/issues/268).

> **Before committing any change described in this roadmap:** run
> `./scripts/run_golden_compares.sh`. All golden JSON baselines must still pass — this applies to
> every milestone below, not just the ones that call it out explicitly.

## Goal

Stop paying `qDebug()` string-formatting cost in hot loops. This is currently the single largest
performance problem in the codebase: removing it makes `DistanceEngine` **43×–72× faster** on real
networks — roughly an order of magnitude more than WS3 M1's entire 24-core parallelisation won
(2.7×–8.3×).

This workstream exists because a specific problem was measured, not because the logging "could be
cleaner". Origin: the *"Secondary finding, still open"* filed against #254 in
[`roadmap_ws3_architecture_performance.md`](roadmap_ws3_architecture_performance.md) — "filed
separately if it turns out to matter." It turned out to matter, by a lot.

## Status at a Glance

| Milestone | Status |
|---|---|
| L1 — Release builds quiet by default | 🔴 Not started |
| L2 — `DistanceEngine` → logging category | 🔴 Not started |
| L3 — Parsers → logging category | 🔴 Not started |
| L4 — `matrix.cpp` → logging category (+ decide fate of the unreachable Gauss-Jordan inverse) | 🔴 Not started |

---

## The mechanism (why `qDebug()` is not free even when output is off)

`qCDebug(category)` expands to a `for`-loop holder that never enters when the category is disabled,
so **its streaming arguments are never evaluated**
(`QT_MESSAGE_LOGGER_COMMON`, `qloggingcategory.h:159`). Plain `qDebug()` expands to an unconditional
`QMessageLogger(...).debug` call (`qlogging.h:168`): the `QDebug` object is constructed, every `<<`
argument is evaluated and formatted into a `QString`, and only *then* is the result dropped by the
filter rule or message handler.

Measured on this machine (macOS arm64, Qt 6.10.1, Release `-O2`), representative inner-loop payload
(4 ints + 2 doubles + literals):

| Form | ns/call |
|---|---|
| `qDebug()` with `default.debug=false` filter rule | **618.65** |
| `qDebug()` with output enabled, stderr → `/dev/null` | **2193.30** |
| `qCDebug(disabledCategory)` | **0.40** |
| empty-loop baseline | 2.40 |

**1544× apart.** Suppressing output — by filter rule *or* by message handler — does not help; only
the category short-circuit (or compiling the call out) does.

## Evidence

Two Release builds from `develop`, identical except `-DQT_NO_DEBUG_OUTPUT` on the second (used
purely as a **measurement instrument** — see "Why not just define `QT_NO_DEBUG_OUTPUT`" below).
Networks from `~/socnetv/library/nets/large/` (not shipped with the repo).

| Case | base | debug compiled out | speedup |
|---|---|---|---|
| 1000N/10000A, distances only | 4876 ms | 112 ms | **43.5×** |
| 1000N/10000A, + centralities | 7605 ms | 134 ms | **56.8×** |
| ER undirected N1000/E19879, + centralities | 17186 ms | 237 ms | **72.5×** |
| 2000N/40000E, distances only | 51672 ms | **943 ms** | **54.8×** |
| GraphML load, 1000 nodes | 88 ms | 28 ms | 3.1× |
| GraphML load, 2000 nodes | 335 ms | 116 ms | 2.9× |

Binary size: 2.39 MB → 1.61 MB (−33 %).

**Correctness:** `run_golden_compares.sh` (all baselines) and `run_golden_io_roundtrip.sh` pass
unchanged on the debug-free build — the work is genuinely being done, not skipped.

**The measurement is conservative.** `socnetv-cli` already discards debug output — `socnetv_cli.cpp:88`
installs a handler that drops `QtDebugMsg` entirely. So the table above is **pure formatting cost
with the output thrown away**, at the 618 ns/call rate. The GUI's default path (L1) runs at the
2193 ns/call rate, so its win should be larger still.

Reproduce with:

```bash
cmake -S . -B build-nodbg -DCMAKE_PREFIX_PATH=... -DBUILD_CLI=ON -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_CXX_FLAGS_RELEASE="-O2 -DNDEBUG -DQT_NO_DEBUG_OUTPUT"
```

## Live call-site inventory

Counted excluding commented-out lines (a naive `grep qDebug` over-counts badly — most of the canvas
hits are already commented out). **1645 live calls** across `src/`, of 1838 textual matches.

| File | live | note |
|---|---|---|
| `mainwindow.cpp` | 301 | mostly not hot paths |
| `parser/parser_graphml.cpp` | 99 | L3 |
| `matrix.cpp` | 96 | L4 |
| `parser/parser_dl.cpp` | 88 | L3 |
| `graph/reporting/graph_reports.cpp` | 85 | |
| `engine/distance_engine.cpp` | 75 | **L2 — the measured 43×–72×** |
| `graph/storage/graph_vertices.cpp` | 56 | 2 inside `vertices()`'s O(N) loop |
| `graphicswidget.cpp` | 12 | already `qCDebug(lcGW)` |
| `graphicsnode.cpp` | 1 | already clean |
| `graphicsedge.cpp` | 0 | already clean |

`lcGW` in `graphicswidget.cpp:45` is currently the **only** `Q_LOGGING_CATEGORY` in the tree, and
the canvas classes are the only ones already cleaned up (WS10 Phase 1). It is the pattern to follow.

---

## Milestones

### L1 — Release builds quiet by default

**Problem.** `main.cpp:138` sets `debugLevel = -1` when no `-d` flag is passed. That falls through to
the `default:` case at `mainwindow.cpp:126-129`, which sets a *verbose* message pattern (time,
threadid, file, line, function) and **never disables output**. Only an explicit `-d 0` reaches the
`setFilterRules("default.debug=false")` branch at `mainwindow.cpp:115`.

So every normal GUI launch runs all 1645 live sites at the 2193 ns/call rate, writing to stderr.

**Approach.** Make the no-flag default behave like the current `-d 0`: quiet pattern + filter rules
disabling `default.debug` and `socnetv.*.debug`. Keep `-d 1` / `-d 2` working exactly as today.

**Note:** L1 alone is *not* sufficient — it moves calls from 2193 ns to 618 ns, roughly 3.5× of the
~70× available. L1 is the cheap default-safety fix; L2–L4 are where the speedup actually comes from.
Do L1 first anyway: it is one branch, it is the highest value-to-effort item here, and it makes the
shipped default sane.

**Completion criteria:** launching without `-d` produces no debug output; `-d 1` and `-d 2` are
unchanged; golden scripts pass.

### L2 — `DistanceEngine` → dedicated logging category

**This is the milestone that carries the measured 43×–72×.** 75 live calls, zero `qCDebug`.
The ones that matter:

- `distance_engine.cpp:1279-1469` — dijkstra's inner edge-relaxation loop, ~20 calls, of which
  lines 1314, 1324, 1346 plus one of 1333/1339 are **unconditional per surviving edge** → fires
  O(V·E) times.
- `distance_engine.cpp:1052-1158` — the BFS inner loop, same shape.
- `distance_engine.cpp:698-726` — `finalize()`'s O(N²) vertex-pair loop, unconditional `qDebug()`
  per pair (≈4M calls at N=2000).

**Approach.** Add `Q_LOGGING_CATEGORY(lcEngine, "socnetv.engine")`, convert all 75 sites, same
pattern as `lcGW`. Purely mechanical — no logic or control-flow change.

**Completion criteria:** golden + io_roundtrip pass unchanged; benchmark the same four large-network
cases from the Evidence table and report actual before/after numbers, matching WS3 M1's evidence
standard. The converted build should land close to the debug-free column.

### L3 — Parsers → logging category

99 live calls in `parser_graphml.cpp`, 88 in `parser_dl.cpp`; other parsers 35–46 each. Measured
~3× on load time (335 → 116 ms for a 2000-node GraphML). Not measurable on shipped `src/data/`
— every file there loads in under a millisecond, so this needs the large-net directory.

**Completion criteria:** golden + io_roundtrip pass; report measured `LOAD_MS` before/after on a
large GraphML and a large Pajek file.

### L4 — `matrix.cpp` → logging category, and decide the fate of the Gauss-Jordan inverse

96 live calls. Two distinct sub-problems:

- **Live cost:** `lubksb()`'s 3 calls, invoked once per column from `inverse()` → O(N²) formatting
  on the Information Centrality path (`graph_centrality.cpp:128`, `invM.inverse(WM)`).
- **Latent landmine:** `matrix.cpp:1057-1074` — `inverseByGaussJordanElimination()`'s innermost
  `for k` loop carries **4 unconditional `qDebug()` calls**, three of which compute arithmetic
  (`elim_coef * A.item(j,k)`) *solely in order to print it*. That is O(N³) formatting: ≈4×10⁹ calls
  at N=1000, on the order of 40 minutes of pure string building.

  **It is currently unreachable.** The only caller passes `"lu"`
  (`graph_reports.cpp:5780`) and the parameter default is also `"lu"` (`graph.h:823`). So this is
  not a live cost today — but it is one `"gauss"` away from being catastrophic.

**Decision required (coordinate with [WS5](roadmap_ws5_matrices_modernization.md)):** either delete
`inverseByGaussJordanElimination()` outright as dead code, or keep it and convert its logging like
everything else. Do not leave it as-is. WS5's A5 currently plans to add cancellation support to this
method — that plan should be revisited in light of it having no reachable caller.

**Completion criteria:** golden + io_roundtrip pass; a decision recorded here on the Gauss-Jordan
method; measured before/after on an Information Centrality run over a large network.

---

## Why not just define `QT_NO_DEBUG_OUTPUT` in release builds?

Because it would silently break the `-d 1` / `-d 2` command-line feature. `QT_NO_DEBUG_OUTPUT`
compiles `qDebug` to a no-op sink at preprocessor level, so no runtime `setFilterRules()` call could
ever bring the messages back — users could no longer produce a debug log for a bug report from a
shipped build.

The `qCDebug` + category route gets the same performance (0.40 ns/call when disabled, i.e.
effectively free) while keeping the messages runtime-enablable via the existing
`QLoggingCategory::setFilterRules("socnetv.*.debug=true")` path that `-d 2` already uses. That is
why L2–L4 are conversions, not deletions, and why the `QT_NO_DEBUG_OUTPUT` build in the Evidence
section is described as a measurement instrument rather than the proposed fix.

## Risk / blast radius

Low per change, but wide: ~1645 call sites. Every conversion is mechanical (`qDebug()` →
`qCDebug(lcX)`) with no control-flow change, and the golden harness covers the computational result
of every affected path. Mitigation is sequencing, not cleverness: **one module per commit,
golden-verified after each**, largest measured win first.

The one non-mechanical judgement call is L4's Gauss-Jordan decision.

## Not verified

- **GUI-side end-to-end effect of L1 and L2.** All timings above are from `socnetv-cli` (same engine
  code, headless). The 2193 ns/call GUI figure is a standalone microbenchmark, not the real app. Per
  `CLAUDE.md`, confirming the GUI-facing win requires driving `SocNetV.app` via
  `--interactive-script` (see [`roadmap_ws12_cli_scripting_mode.md`](roadmap_ws12_cli_scripting_mode.md))
  and reporting real evidence. Worth doing as part of L1/L2 rather than assuming the CLI number
  transfers.
- **macOS-from-Finder stderr destination.** When launched from Finder rather than a terminal, stderr
  goes to `os_log`, which is expected to be more expensive than the `/dev/null` figure measured
  here. Not measured.

## Work Rules

- Performance claims must be measured, not asserted — this roadmap exists because a number was
  produced, and every milestone above carries its own measurement requirement.
- One module per commit, `run_golden_compares.sh` clean after each.
- Conversions only: do not "tidy" the log messages themselves while converting them. A pure
  mechanical diff is what makes 1645 sites reviewable.
