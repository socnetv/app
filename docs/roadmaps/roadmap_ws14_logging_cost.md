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
| L1 — Release builds quiet by default | 🟢 Done (2026-07-31) |
| L2 — `DistanceEngine` → logging category | 🟢 Done (2026-07-31) |
| L3 — Parsers → logging category | 🔴 Not started |
| L4 — `matrix.cpp` → logging category (+ decide fate of the unreachable Gauss-Jordan inverse) | 🔴 Not started |

See "L1/L2 real-world evidence" below for measured before/after numbers (CLI and GUI) and two real
bugs found while wiring up the GUI-side measurement — both are why the numbers below ended up
larger than this doc originally predicted.

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
| `engine/distance_engine.cpp` | 75 → **78** | **L2 — the measured 43×–72×, done.** Original count was
  a `grep "qDebug()"` literal match, which undercounts: 3 calls inside the BFS loop
  (`distance_engine.cpp:1080-1088`) use the printf-style `qDebug("...", args)` overload with no
  literal `()`. Same hot path as the already-covered Dijkstra loop; converted along with the rest. |
| `graph/storage/graph_vertices.cpp` | 56 | 2 inside `vertices()`'s O(N) loop |
| `graphicswidget.cpp` | 12 | already `qCDebug(lcGW)` |
| `graphicsnode.cpp` | 1 | already clean |
| `graphicsedge.cpp` | 0 | already clean |

`lcGW` in `graphicswidget.cpp:45` is currently the **only other** `Q_LOGGING_CATEGORY` in the tree
besides `lcEngine` (added by L2), and the canvas classes were the only ones already cleaned up
(WS10 Phase 1) before this pass. It is the pattern L2 followed.

---

## L1/L2 real-world evidence (2026-07-31)

Measured on this machine (macOS arm64, Qt 6.10.1, Release `-O2`), same build directory rebuilt
incrementally at each phase, `run_golden_compares.sh` + `run_golden_io_roundtrip.sh` clean after
every step.

### Two real bugs found while wiring up the GUI-side measurement

Both existed in code this doc's original L1 description didn't anticipate, and both made the
naive before/after numbers meaningless until fixed:

1. **A no-flag launch didn't actually come up quiet — it inherited whatever was last persisted.**
   `MainWindow::initSettings()` runs immediately after the constructor's `debugLevel` switch and,
   for the no-flag case, left `appSettings["printDebug"]` untouched — so a plain launch silently
   re-applied whatever the Settings-dialog "print debug messages" checkbox last saved to
   `settings.conf`. One past toggle-on (e.g. during a debugging session) made every future no-flag
   launch pay full cost forever — on this machine, `printDebug=true` was already persisted before
   this session started, meaning the "before" numbers below reflect a real, currently-live bug, not
   a contrived worst case. Fixed in `initSettings()`: a no-flag launch now forces `printDebug=false`
   the same way `-d 0` already did, regardless of persisted state. The Settings dialog checkbox
   still works fine as a live, in-session toggle.

2. **`qSetMessagePattern("")` (the pre-existing `-d 0` quiet pattern) silently ate *all* output, not
   just debug spam.** A truly empty pattern has no `%{message}` token, so Qt's formatter reduces
   *every* message — debug, info, warning, critical, any category — to an empty string. This had
   apparently always been true of `-d 0`; nobody noticed because nothing in the tree used `qInfo()`
   before this session added the `distances`/`distances centralities` benchmark commands (see
   [`roadmap_ws12_cli_scripting_mode.md`](roadmap_ws12_cli_scripting_mode.md)), whose own `qInfo()`
   instrumentation went dark under it. Fixed: `qSetMessagePattern("%{message}")` — bare content,
   still correctly suppressible at debug level via the separate filter-rule mechanism, but no longer
   swallowing `qWarning()`/`qCritical()`/`qInfo()` a user might actually need to see.

3. **(L2-specific) `socnetv-cli`'s quiet mode never actually engaged `qCDebug()`'s cost-saving gate.**
   `socnetv_cli.cpp`'s non-verbose path only ever installed a `qInstallMessageHandler` that discards
   `QtDebugMsg`/`QtInfoMsg` *after* the message is already formatted — it never called
   `QLoggingCategory::setFilterRules()`, which is the thing that actually lets `qCDebug(category)`
   skip argument evaluation *before* formatting. A converted category defaults to enabled, so without
   this the L2 conversion was a measured no-op in the CLI harness: `qCDebug(lcEngine)` still paid
   full formatting cost, just to have the CLI's handler throw the result away. Found by benchmarking
   and getting a suspiciously small delta instead of anything near 43×; fixed by adding the matching
   `setFilterRules("default.debug=false\nsocnetv.*.debug=false")` call alongside the existing
   handler.

### CLI (Release, `socnetv-cli --bench`, `run_benchmarks.sh --type distance`)

| Case | Before (pre-WS14 / broken-L2-no-filter-rule, ~equivalent) | After L1+L2 | Speedup | This doc's original prediction |
|---|---|---|---|---|
| 1000N/10000A, distances only | 4674 ms | 108 ms | **43.3×** | 112 ms (`QT_NO_DEBUG_OUTPUT` instrument) |
| 1000N/10000A, + centralities | 7364 ms | 125 ms | **58.9×** | 134 ms (`QT_NO_DEBUG_OUTPUT` instrument) |

L1 has no effect on the CLI number (the CLI already discarded debug output regardless of filter
rules via its own message handler, independent of `mainwindow.cpp`); the delta above is entirely
L2 + the `socnetv_cli.cpp` fix. Both land within ~4% of this doc's original `QT_NO_DEBUG_OUTPUT`
predictions — the real `qCDebug` conversion delivers what the measurement instrument predicted it
would.

### GUI (real app, `--interactive-script`, no `-d` flag — the actual thing #1 above was about)

Smaller network (`Random_ER_Undir_N300_E1735.graphml`, not the 1000N/2000N CLI cases) — chosen so
the noisy "before" case finishes in ~15s instead of several minutes; see
[`roadmap_ws12_cli_scripting_mode.md`](roadmap_ws12_cli_scripting_mode.md) for the `distances`/
`distances centralities` commands used. Each "before"/"after L1" number reproduced twice
independently (same run twice, `printDebug=true` restored in `settings.conf` between runs to
correctly reproduce bug #1 above — running the app mutates that file, which is itself a real trap:
see commit history for the full story).

| Case | Before | After L1 only | After L1+L2 | L1-alone speedup | Total speedup |
|---|---|---|---|---|---|
| distances only | 13588 ms avg | 482 ms avg | 27 ms | **~28.2×** | **~503×** |
| distances + centralities | 17643 ms avg | 725 ms | 29 ms | **~24.3×** | **~608×** |

**L1 alone measured far above this doc's original ~3.5× prediction** (which only modeled the
per-call formatting-cost delta, 2193ns → 618ns/call). Likely explanation: `DistanceEngine`'s
parallel worker threads (WS3 M1) all writing to stderr concurrently hit real contention on Qt's
internal logging lock — suppressing output removes that contention entirely, on top of the
formatting-cost delta this doc already accounted for. Not yet independently isolated/profiled;
noted here as the leading hypothesis, not a confirmed root cause.

This closes the "Not verified: GUI-side end-to-end effect of L1 and L2" item below.

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

**Done (2026-07-31).** Both the fix above and two additional real bugs found while implementing it
— see "L1/L2 real-world evidence" above for the full writeup and measured numbers (~28×/~24× from
L1 alone on the GUI, far above the ~3.5× estimated below before the parallel-stderr-contention
effect was known). Landed as two commits: the `distances`/`distances centralities` interactive-script
benchmark scaffolding, then the L1 fix itself.

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

**Done (2026-07-31).** 78 sites converted, not 75 — the printf-style overload inside the BFS loop
was undercounted, see "Live call-site inventory" above. Also required a fix to `socnetv_cli.cpp`
(it never called `setFilterRules()`, so the category gate was never actually engaged there — see
"L1/L2 real-world evidence" above) without which this milestone would have measured as a no-op.
CLI numbers land within ~4% of this doc's original `QT_NO_DEBUG_OUTPUT`-instrument predictions
(108ms vs 112ms, 125ms vs 134ms for the two 1000N/10000A cases).

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

- ~~GUI-side end-to-end effect of L1 and L2.~~ **Verified 2026-07-31** — see "L1/L2 real-world
  evidence" above. Confirmed via real `SocNetV.app` runs driven by `--interactive-script`, not
  extrapolated from the CLI number; the GUI win turned out larger than the CLI one (~503×/~608×
  vs ~43×/~59×), not merely "transferred", because of the parallel-stderr-contention effect noted
  there.
- **macOS-from-Finder stderr destination.** When launched from Finder rather than a terminal, stderr
  goes to `os_log`, which is expected to be more expensive than the `/dev/null` figure measured
  here. Not measured — all GUI runs above launched the binary directly from a terminal/script, not
  via Finder double-click.
- **Parallel-stderr-contention hypothesis (L1's ~28×/~24× vs the ~3.5× originally predicted).** Named
  as the leading explanation in "L1/L2 real-world evidence" above but not independently isolated —
  would need a profiler run comparing single-threaded vs `DistanceEngine`'s actual multi-threaded
  source loop with output enabled, holding everything else fixed, to confirm the lock-contention
  mechanism specifically rather than some other factor.

## Work Rules

- Performance claims must be measured, not asserted — this roadmap exists because a number was
  produced, and every milestone above carries its own measurement requirement.
- One module per commit, `run_golden_compares.sh` clean after each.
- Conversions only: do not "tidy" the log messages themselves while converting them. A pure
  mechanical diff is what makes 1645 sites reviewable.
