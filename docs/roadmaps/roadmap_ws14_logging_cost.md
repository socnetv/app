# Logging Cost & Release-Build Hygiene Roadmap (WS14)

Tracking issue: [#268](https://github.com/socnetv/app/issues/268).

## Goal

Stop paying `qDebug()` string-formatting cost in hot loops — the single largest performance problem
found in the codebase to date. Origin: a "secondary finding, still open" filed against #254 in
[`roadmap_ws3_architecture_performance.md`](roadmap_ws3_architecture_performance.md).

## Status

✅ Done. Zero `qDebug()` calls remain anywhere in `src/`, live or commented-out — converted to
`qCDebug(category)` throughout (28 categories, ~1,760 call sites). The logging-category convention
(why it's fast, how to add a new category) now lives in
[`README_DEVELOPER_NOTES.md`](../README_DEVELOPER_NOTES.md)'s "Logging" section, not here.

## What WS14 Delivered

- **L1 — release builds quiet by default.** A no-flag launch previously inherited whatever debug
  setting was last persisted to `settings.conf` — fixed to always force quiet, matching `-d 0`.
- **L2 — `DistanceEngine` → `lcEngine`.** The actual hot path: unconditional `qDebug()` in the
  BFS/Dijkstra inner loops and `finalize()`'s O(N²) pair loop.
- **L3 — parsers → shared `lcParser`.**
- **L4 — `matrix.cpp` → `lcMatrix`.** Also settled a standing question: `inverseByGaussJordanElimination()`
  has no reachable caller (`createMatrixAdjacencyInverse()`'s sole caller always passes `"lu"`) —
  decision was to keep and convert it rather than delete it (coordinated with
  [WS5](roadmap_ws5_matrices_modernization.md), which owns any future decision to actually remove
  dead matrix code).
- **Full tree sweep** — every remaining call site beyond the four hot-path milestones, at the user's
  request: even cold-path calls pay full formatting cost whenever debug output is ever turned on.

**Measured results** (MacBook Pro M5, 24GB RAM, Release `-O2`, golden-clean throughout):

| What | Before | After | Speedup |
|---|---|---|---|
| `DistanceEngine`, CLI, 1000N/10000A | 4,674 ms | 108 ms | **43×** |
| `DistanceEngine`, GUI, distances+centralities | 17,643 ms | 29 ms | **~608×** (compounds L1's parallel-stderr-contention fix with L2's category gate — see Notes) |
| Parser load, 2000-node GraphML | 323 ms | 109 ms | **~3×** |
| Information Centrality, N=300 | 4.27 s | 0.30 s | **~14×** |
| Binary size | 2.39 MB | 1.61 MB | −33% |

`qCDebug(disabledCategory)` measured at 0.40 ns/call vs. plain `qDebug()`'s 618-2,193 ns/call
depending on output destination — a ~1,500× difference in the per-call cost this conversion
removes. Full mechanism and reproduction command in README.

## Notes for the Future

- **The GUI win (~608×) is larger than the CLI win (~43×) for a real reason, not measurement noise:**
  `DistanceEngine`'s parallel worker threads all writing to stderr concurrently hit real contention
  on Qt's internal logging lock; suppressing output (L1) removes that contention entirely, on top of
  L2's formatting-cost fix. Leading hypothesis, not independently profiled/confirmed in isolation.
- **Why not just compile out logging with `QT_NO_DEBUG_OUTPUT`?** It would silently break the
  `-d 1`/`-d 2` command-line feature (compiled-out `qDebug` can never be re-enabled at runtime).
  `qCDebug` + categories gets the same near-zero cost while staying runtime-enablable — why
  conversion, not deletion, was the right fix.
- Two mechanical-conversion gotchas worth remembering if this pattern is ever repeated elsewhere:
  printf-style `qDebug("fmt", args)` and the `qDebug ()` (space-before-parens) variant both evade a
  naive literal-text grep/sed pass — a real conversion needs to account for both.
- Not independently verified: behavior when launched from Finder rather than a terminal (stderr
  routes to `os_log` instead of the destinations measured here — expected to be more expensive, not
  measured).

## What Remains Open

Nothing scoped under WS14. Any *new* code should use `qCDebug(category)` from the start — see
README's "Logging" section for which category to use.
