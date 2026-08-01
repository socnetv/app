# Logging Cost & Release-Build Hygiene Roadmap (WS14)

Tracking issue: [#268](https://github.com/socnetv/app/issues/268).

## Goal

Stop paying `qDebug()` string-formatting cost in hot loops. This is currently the single largest
performance problem in the codebase: removing it makes `DistanceEngine` **43×–72× faster** on real
networks — roughly an order of magnitude more than WS3 M1's entire 24-core parallelisation won
(2.7×–8.3×).

This workstream exists because a specific problem was measured, not because the logging "could be
cleaner". Origin: the *"Secondary finding, still open"* filed against #254 in
[`roadmap_ws3_architecture_performance.md`](roadmap_ws3_architecture_performance.md) — "filed
separately if it turns out to matter." It turned out to matter, by a lot.

## Status

✅ Done — full tree converted, zero `qDebug()` remains.

| Milestone | Status |
|---|---|
| L1 — Release builds quiet by default | 🟢 Done |
| L2 — `DistanceEngine` → logging category | 🟢 Done |
| L3 — Parsers → logging category | 🟢 Done |
| L4 — `matrix.cpp` → logging category (+ decide fate of the unreachable Gauss-Jordan inverse) | 🟢 Done |
| Full tree sweep (beyond L1-L4's original scope) | 🟢 Done |

**The entire tree is now converted — zero `qDebug()` text remains anywhere in `src/`, live or
commented-out.** L1-L4 were the roadmap's originally-scoped hot paths; after they landed, the user
asked for the same treatment applied to every remaining call site, not just the hot-path subset —
28 `Q_LOGGING_CATEGORY` definitions total (one per `src/graph/<domain>/` slice, one per other
standalone file, two shared ones for `src/forms/` and the canvas-item files), ~1760 `qCDebug()` call
sites. See "Full sweep evidence" below for the file-by-file breakdown and the real bugs found along
the way.

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

Measured on a MacBook Pro M5, 24GB RAM (macOS arm64, AppleClang, Qt 6.10.1, Release `-O2`),
representative inner-loop payload (4 ints + 2 doubles + literals):

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

**All converted — zero live `qDebug()` calls remain anywhere in `src/`.** This section originally
tracked per-file counts while L1-L4 and the broader sweep were in progress; that table is gone since
tracking is no longer useful once the answer is "all of them, everywhere." See "Full sweep evidence"
below for the category scheme and the final tally (28 `Q_LOGGING_CATEGORY` definitions, ~1760
`qCDebug()` call sites).

One correction worth keeping: the original count of 1645 live calls (of 1838 textual matches,
excluding comments) was stale in several places by the time the full sweep started — not from the
printf-style undercounting mechanism found in `distance_engine.cpp` (isolated to that file plus
`parser_pajek.cpp`), just plain drift. `matrix.cpp` was actually 118, not 96; `mainwindow.cpp` was
313, not 301. Worth remembering next time a call-site count in this doc is used to estimate scope —
re-verify rather than trust it at face value.

---

## L1/L2 real-world evidence

Measured on a MacBook Pro M5, 24GB RAM (macOS arm64, Qt 6.10.1, Release `-O2`), same build directory
rebuilt incrementally at each phase, `run_golden_compares.sh` + `run_golden_io_roundtrip.sh` clean after
every step.

### Two real bugs found while wiring up the GUI-side measurement

Both existed in code this doc's original L1 description didn't anticipate, and both made the
naive before/after numbers meaningless until fixed:

1. **A no-flag launch didn't actually come up quiet — it inherited whatever was last persisted.**
   `MainWindow::initSettings()` runs immediately after the constructor's `debugLevel` switch and,
   for the no-flag case, left `appSettings["printDebug"]` untouched — so a plain launch silently
   re-applied whatever the Settings-dialog "print debug messages" checkbox last saved to
   `settings.conf`. One past toggle-on (e.g. during a debugging session) made every future no-flag
   launch pay full cost forever — on the MacBook Pro M5 used for this investigation,
   `printDebug=true` was already persisted before this session started, meaning the "before" numbers
   below reflect a real, currently-live bug, not a contrived worst case. Fixed in `initSettings()`: a no-flag launch now forces `printDebug=false`
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

**Done.** Both the fix above and two additional real bugs found while implementing it
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

**Done.** 78 sites converted, not 75 — the printf-style overload inside the BFS loop
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

**Done.** One shared category (`Q_LOGGING_CATEGORY(lcParser, "socnetv.parser")`,
`Q_DECLARE_LOGGING_CATEGORY` in `parser.h`) across all 9 files (~409 sites), not one per file — new
pattern vs. `lcGW`/`lcEngine`'s file-local precedent, deliberate. `parser_pajek.cpp`'s 10 printf-style
calls handled individually by line to avoid touching its 12 (since deleted — no `qDebug()` text
allowed anywhere, not even in comments) commented-out instances of the same pattern. Measured
(`socnetv-cli --kernel distance`, `LOAD_MS`, 2000-node files): GraphML ~323ms → ~109ms (**~3.0×**,
matching this section's original prediction almost exactly), Pajek ~186ms → ~130ms (**~1.4×**, more
modest — Pajek's parser has proportionally fewer `qDebug()` calls relative to per-line work). See
"L3/L4 real-world evidence" below for the full writeup.

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

**Decision made: keep it, convert its logging.** All 118 (not 96, see corrected
inventory below) live `qDebug()` sites in `matrix.cpp`, including the 15 inside
`inverseByGaussJordanElimination()`, converted to `qCDebug(lcMatrix)` — no deletion. WS5's A5
cancellation-support plan for this method is unaffected by this decision either way (still worth
revisiting separately in light of it having no reachable caller, per WS5's own doc).

**Completion criteria:** golden + io_roundtrip pass; a decision recorded here on the Gauss-Jordan
method; measured before/after on an Information Centrality run over a large network.

**Done.** `Q_LOGGING_CATEGORY(lcMatrix, "socnetv.matrix")`, file-local (same pattern as
`lcGW`/`lcEngine`). Two whitespace variants required care: `qDebug ()` (space before parens, ~24
sites) alongside plain `qDebug()` — a literal-text grep undercounts these the same way the L2
printf-style calls were undercounted; handled with one whitespace-tolerant sed pass. Measured
(`socnetv-cli --kernel prominence`, N=300 ER network — chosen over the roadmap's 1000N cases because
Information Centrality's O(N³) matrix inversion made the pre-conversion 1000N run impractically slow
to benchmark; the `qDebug()` formatting cost inside `lubksb()` itself scales O(N²), so the win is
already clearly visible at N=300): **~4.27s → ~0.30s, ~14.2×**.

---

## L3/L4 real-world evidence

Same discipline as L1/L2: measured on the same MacBook Pro M5, 24GB RAM, same build directory rebuilt incrementally
(each "before" number from `git stash`-ing the milestone's still-uncommitted change and rebuilding,
not extrapolated), `run_golden_compares.sh` + `run_golden_io_roundtrip.sh` clean after every commit.

| Milestone | Case | Before | After | Speedup |
|---|---|---|---|---|
| L3 | GraphML load, 2000N/40000E | ~323 ms avg | ~109 ms avg | **~3.0×** |
| L3 | Pajek load, 2000N/40000E | ~186 ms avg | ~130 ms avg | **~1.4×** |
| L4 | Information Centrality, N=300 | ~4.27 s avg | ~0.30 s avg | **~14.2×** |

Two more undercounting variants found and fixed while doing this (on top of L2's printf-style-call
discovery), both from the same root cause — a literal-text `qDebug()` grep only catches one exact
spelling:

- **`parser_pajek.cpp`** has 10 live printf-style `qDebug("...", args)` calls (same overload
  `distance_engine.cpp`'s BFS loop needed) — checked for and confirmed *not* present in any other
  parser file or in `matrix.cpp`.
- **`matrix.cpp`** has ~24 calls written as `qDebug ()` (space before the parens) alongside the
  more common `qDebug()` — a different textual variant than the printf-style one, same underlying
  lesson: a mechanical conversion pass needs whitespace-tolerant matching, not a single literal
  string search, or it silently leaves live call sites unconverted.

Also found and fixed: `graphicswidget.cpp`'s clean rebuild (done after L3, to size up its warning
impact before the rest of the sweep) surfaced **438 instances of `-Wvariadic-macro-arguments-omitted`**
— every `qCDebug(category) << "message"` call site (the standard streaming idiom, used by every
conversion so far) passes zero arguments to `qCDebug`'s variadic macro parameter, legal in C++20 but
relying on a GNU/Clang extension on this project's C++17 standard, which `-Wpedantic`
(`CMakeLists.txt:337`) flags. Harmless — this is how the pre-existing `lcGW` category already
worked, nobody had checked for it — but would have become 1000+ warnings by the time the rest of the
sweep is done. Fixed with a targeted `-Wno-variadic-macro-arguments-omitted` alongside the existing
`-Wall -Wextra -Wpedantic`, landed as part of the L3 commit. Confirmed via clean rebuild: 505 → 67
total warnings, all 438 variadic-macro instances gone, remaining 67 pre-existing and unrelated
(`-Winconsistent-missing-override`/`-Wdeprecated-declarations` in untouched files).

---

## Full sweep evidence

Beyond L1-L4, every remaining live `qDebug()` call site in `src/` was converted, at the user's
request — not because the roadmap's original scope required it, but because even non-hot-path calls
still pay full formatting cost whenever debug output is ever turned on (a `-d 1`/`-d 2` session, or
the Settings checkbox), and because a genuinely "no qDebug() left anywhere" codebase is easier to
reason about than "hot paths converted, the rest still plain qDebug()". Same discipline throughout:
mechanical conversions only, one logical module per commit (~30 commits total for this tier),
`run_golden_compares.sh` + `run_golden_io_roundtrip.sh` clean after every one. No before/after
timing was collected per-file for this tier (the roadmap already established these aren't hot
loops — see the per-file hot/cold classification in "Live call-site inventory" above); a golden pass
is the bar here, not a speedup number.

### Category scheme

- **One category per `src/graph/<domain>/` slice directory** (16 of them: `centrality`,
  `clustering`, `cohesion`, `crawler`, `distances`, `filters`, `generators`, `io`, `layouts`,
  `matrices`, `prominence`, `reachability`, `relations`, `reporting`, `similarity`, `ui`), plus
  `lcGraph` for `graph.cpp` itself and `lcGraphCore` for the `core/` slice — all declared in `graph.h`
  (already included by every slice `.cpp`) and defined once in `graph.cpp`, same reasoning as
  `lcParser`.
- **File-local categories** for other standalone files with their own `qDebug()` calls:
  `lcMainWindow` (`mainwindow.cpp`), `lcWebCrawler` (`webcrawler.cpp`, distinct from
  `graph/crawler/`'s `lcGraphCrawler` — different file, different job), `lcChart` (`chart.cpp`),
  `lcTextEditor` (`texteditor.cpp`).
- **`lcGW` promoted from file-local to shared**: `Q_DECLARE_LOGGING_CATEGORY` moved to
  `graphicswidget.h`, reused by the five small canvas-item files (`graphicsnode.cpp`,
  `graphicsedgelabel.cpp`, `graphicsedgeweight.cpp`, `graphicsnodelabel.cpp`,
  `graphicsnodenumber.cpp`) that each had only 0-1 live calls — not enough to justify a category of
  their own.
- **New shared `lcForms`**: 22 `src/forms/*.cpp` dialog files (1-16 calls each, pure one-shot UI
  code), no pre-existing shared header to piggyback on, so a small purpose-built
  `src/forms/forms_logging.h` was added — same reasoning as `lcParser`/`lcGW`.
- **`graphvertex.cpp`** (outside `src/graph/` but part of the core data model) and
  **`tools/headless_graph_loader.cpp`** (CLI-only, part of the load pipeline) both reuse an existing
  `src/graph/` category (`lcGraphCore`, `lcGraphIO`) rather than getting their own for a single call
  site each.

### Real problems found while doing this

1. **`graphicswidget.cpp` was never actually fully converted**, despite the original inventory table
   listing it as done — 14-15 live calls remained. Corrected as part of the cleanup pass.
2. **A second undercounting variant beyond L2's printf-style discovery**: `qDebug ()` with a space
   before the parens (found in `matrix.cpp`, ~24 sites) — a literal `qDebug()` grep misses this too.
   Both `matrix.cpp` and `mainwindow.cpp` needed line-by-line handling for their printf-style calls
   on top of the whitespace-tolerant pass.
3. **The "no `qDebug()` text anywhere, not even in comments" policy surfaced a process bug**: the
   per-file dead-comment-deletion step was, in several early files, run *after* the
   `qDebug()`→`qCDebug()` rename — so it searched for text that no longer existed and silently
   matched nothing, leaving dead comments alive under their new (renamed) spelling. Caught by a
   dedicated whole-tree audit; fixed with a script that matches both `// qDebug(` and `// qCDebug(`
   as dead-statement starts (handling multi-line streamed comments too), removing 291 lines across
   20 files. A second, related bug: a blind `sed` conversion pass doesn't distinguish live code from
   *prose* comments that merely mention `qDebug()` as a term (e.g. this document's own commit
   messages, or explanatory code comments written during this sweep) — several of the session's own
   explanatory comments got their generic "qDebug()" references incorrectly rewritten to a
   file-specific category name (e.g. `qCDebug(lcMainWindow)`) by the mechanical rename; caught by
   manual review before committing, not by any automated check.

### Final numbers

- **28 `Q_LOGGING_CATEGORY` definitions**, **~1760 `qCDebug()` call sites**, **zero live `qDebug()`
  calls anywhere in `src/`** (verified by a final whole-tree grep with no exclusions needed beyond
  a handful of legitimate prose comments).
- Clean-rebuild warning count: 67 (all pre-existing, unrelated to this workstream) — confirms the
  `-Wno-variadic-macro-arguments-omitted` fix from L3 scales correctly across the full conversion,
  not just the milestone it landed with.
- `run_golden_compares.sh`, `run_golden_io_roundtrip.sh` (19/19), and `run_benchmarks.sh` (all three
  types: distance, io, clustering) all clean on the final build.

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

- ~~GUI-side end-to-end effect of L1 and L2.~~ **Verified** — see "L1/L2 real-world
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
