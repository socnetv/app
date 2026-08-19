# CLI Interactive/Scripting Mode (WS12)

## Goal

Drive SocNetV from the command line without manual clicking — profiling with `sample`/`perf`,
exercising GUI-triggered flows the headless `socnetv-cli` tool can't reach (it doesn't build the
GUI at all), automated demos/regression scenarios, and eventually external programs driving
SocNetV as a live component.

## Status

🚧 In progress. Thirty commands shipped across #261/#262/WS14/WS6.6/WS16 — see What WS12
Delivered below; every command now logs a uniform `BENCH` line on completion. Eventually most of
SocNetV's functions should be reachable through interactive mode — see Background for where this
is headed.

## Background

### Long-term direction

The end state: most of SocNetV's functionality reachable through interactive mode, and SocNetV
running as a long-lived process continuously fed commands by a third-party program — e.g.
`tail -f events.txt | socnetv --interactive-script -`, with SocNetV acting as a live visual
monitor for an external process (event spreading, simulations, etc.), not just a one-shot script
runner.

**This is achievable, and the current architecture is most of the way there** — every command
already dispatches through the real Qt event loop rather than a blocking loop, which is the part
that would be hardest to retrofit later. What's actually missing:

- **Streaming input.** `runInteractiveScript()` currently does `readAll().split('\n')` once at
  startup and stops when the list is exhausted. A monitor mode needs the opposite: read what's
  available now, then keep watching indefinitely instead of terminating. For a live-appended file,
  `QFileSystemWatcher` + seeking to the last-read position covers it; for a pipe (the `tail -f |
  socnetv` case), `QSocketNotifier` on stdin's file descriptor, triggering a read whenever data
  arrives. `processNextInteractiveCommand()`'s "no more commands, stop" path becomes "no more
  commands *right now*, go idle until woken" instead.
- **A `-` path convention** for `--interactive-script` to mean "read from stdin," matching the
  usual Unix convention, for the piped use case specifically.
- **Bidirectional communication**, for the monitor use case specifically (not needed for
  scripted testing). Every command shipped so far is one-way: SocNetV consumes and acts, nothing
  reports state back. A real external monitor (e.g. "how many nodes are currently infected")
  needs SocNetV to answer queries, not just receive commands — that's a materially different,
  larger design question (a query/response command syntax, or a different transport entirely)
  than continuous command consumption, and not scoped yet.

## What WS12 Delivered

### CLI flags

- `--encoding <name>` — loads the startup file with a given text codec, bypassing the "Preview
  file & Choose Encoding" dialog.
- `--interactive-script <path>` — runs a plain-text script after startup, one command per line.
  Implementation in `MainWindow::runInteractiveScript()`/`processNextInteractiveCommand()`
  (`mainwindow.cpp`). Three dispatch shapes recur, all documented in the Doxygen comment on
  `processNextInteractiveCommand()` itself. Whichever shape a command uses, the rule is always the
  same: only advance to the next command once this command's own work has genuinely finished,
  never merely queued or triggered — getting this wrong is a real, reproducible bug, not a style
  preference (see below).
  - **No dispatch** (`new`, `render`, `bulk-node-size`, `bulk-edge-color`): a direct, blocking call
    on the GUI thread, no cross-thread queuing — genuinely done by the time the call returns, so
    advancing immediately afterward is correct as-is.
  - **Single-step** (`relation`, `erdos`, `erdos-m`, `save`, `add-node`, `add-edge`,
    `add-relation`, `click-node`, `move`): `QMetaObject::invokeMethod(activeGraph, lambda,
    Qt::QueuedConnection)` queues a lambda onto `activeGraph`'s own thread and returns immediately,
    without waiting for it to finish — so `BENCH` logging *and* the call advancing to the next
    script command (via a nested `QMetaObject::invokeMethod(this, ..., Qt::QueuedConnection)` back
    to the GUI thread) both happen *inside* that lambda, at actual completion, never around the
    `invokeMethod` call itself. Advancing outside the lambda let the next script command (e.g.
    `quit`, or another queued command) race ahead while the previous one's queued work was still
    running, confirmed via an out-of-bounds crash when `quit` ran immediately after `erdos` with no
    `delay` between them. Found and fixed across all 9 affected commands during the WS15
    investigation that also produced Finding 8's fix — see
    `roadmap_ws15_cancellation_progress_unification.md`.
  - **Two-step** (`filter_ego`, `filter_isolates`, `symmetrize_strongties`,
    `symmetrize_cocitation`, `unilateral`, `distances`, `distances_bench` — anything long enough to
    want a progress dialog): `runGraphOperationAsync(operation, waitMessage, onComplete)` — one
    lambda does the (possibly slow) work, a second runs only once that's genuinely finished, to log
    `BENCH` and advance the script. Both lambdas share timer/result state via `std::shared_ptr`,
    since a plain local variable wouldn't survive between two separate lambdas.

### Output format

Every command logs exactly one line on completion:
```
BENCH <command> [command-specific fields] N=<node count> E=<edge count> elapsed_ms=<N>
```
via `qInfo()` — deliberately not `qDebug()`/`qCDebug()`, so these lines keep printing regardless of
logging-category filter state (quiet-by-default, `-d` flags, etc.). This is uniform across every
command below, not just the ones originally added for benchmarking.

### Commands

- `delay X` — wait X seconds before the next command. `elapsed_ms` in its `BENCH` line should read
  ~= the requested delay — a cheap sanity check that scripted delays aren't drifting under load.
- `new` — File → New.
- `relation N` — switch to relation N.
- `unilateral` — toggle unilateral edges. Calls `Graph::edgeFilterUnilateral()` directly via
  `runGraphOperationAsync`, matching `slotEditFilterEdgesUnilateral()`'s own dispatch (two-step
  pattern, see above) rather than triggering the real `QAction`.
- `erdos N p directed|undirected` — generates an Erdős–Rényi `G(n,p)` network.
- `save path` — saves the current network as GraphML.
- `add-node` — adds a node at a random position.
- `add-edge source target [weight]` — adds a directed edge (default weight 1).
- `add-relation name` — adds a new relation and switches to it.
- `distances [weights] [inverse] [dropisolates] [csv]` — mirrors the real Cohesion → Distances
  Matrix menu action (`slotAnalyzeMatrixDistances()`) exactly: same computation (`writeMatrix()` →
  `graphMatrixDistanceGeodesicCreate()`), same `runGraphOperationAsync()` dispatch, same output
  file — just without opening a `TextEditor` afterward, and without `askAboutEdgeWeights()`'s modal
  prompt (the tokens answer what it would ask). Trailing tokens are order-independent; presence of
  a token means true, absence means false. `csv` selects `ReportFormat::Csv` explicitly (WS16,
  #113) rather than reading the persisted Settings preference — a script has no Settings dialog to
  reflect. Previously called `graphDistancesGeodesic()` directly and crashed on some networks
  (`DistanceEngine::initRun` → `Graph::isSymmetric` → `edgeExists` → `GraphVertex::hasEdgeTo`,
  invalid `QMultiHash` access) — the real menu action, computing via `writeMatrix()`, did not crash
  on the same network, so this command now goes through that path instead.
- `distances_bench [weights] [inverse] [dropisolates] [centralities]` — benchmarking-only sibling
  of `distances`: same dispatch and computation, no disk write. `centralities` has no real-menu
  equivalent (the GUI computes each centrality index via ~9 separate menu actions, not one combined
  action), so it lives here rather than on `distances`.
- `report-centrality-degree [weights] [dropisolates] [csv]`,
  `report-centrality-closeness [weights] [inverse] [dropisolates] [csv]`,
  `report-centrality-closeness-ir [weights] [inverse] [dropisolates] [csv]`,
  `report-centrality-betweenness [weights] [inverse] [dropisolates] [csv]`,
  `report-centrality-stress [weights] [inverse] [dropisolates] [csv]`,
  `report-centrality-eccentricity [weights] [inverse] [dropisolates] [csv]`,
  `report-centrality-power [weights] [inverse] [dropisolates] [csv]`,
  `report-centrality-information [weights] [inverse] [csv]`,
  `report-centrality-eigenvector [weights] [inverse] [csv]`,
  `report-prestige-degree [weights] [dropisolates] [csv]`,
  `report-prestige-proximity [dropisolates] [csv]`,
  `report-prestige-pagerank [dropisolates] [csv]` — each mirrors its real `Analyze` menu action
  exactly (`slotAnalyzeCentralityDegree()`, `slotAnalyzeCentralityCloseness()`, etc.), same
  `distances`-style pattern and `csv` token. `report-centrality-degree` was added for WS16 (#113,
  CSV report export) Step 0 as the first centrality/prestige report ever exercised headlessly; the
  other 11 followed in Step 2, once every `writeCentrality*`/`writePrestige*` function gained CSV
  support. `report-centrality-information` and `report-centrality-eigenvector` have no
  `dropisolates` token (the underlying functions don't take one, or - Eigenvector - never blank
  isolate rows regardless); `report-prestige-proximity` and `report-prestige-pagerank` have no
  `weights`/`inverse` tokens (fixed in the real menu action too).
- `render` — forces a synchronous `graphicsWidget->viewport()->repaint()` (unlike `update()`,
  which only schedules one). Added for WS6.6's canvas rendering-perf kernel
  (`roadmap_ws6_testing_ci_regression.md`).
- `bulk-node-size <N>` — calls `slotEditNodeSizeAll(N)` directly (nonzero `N` skips its modal
  `QInputDialog`).
- `bulk-edge-color <name>` — calls `slotEditEdgeColorAll(QColor(name))` directly (a valid color
  skips its modal `QColorDialog`).
- `move <node> <x> <y>` — sets an absolute canvas position via `Graph::vertexPosSet()`. Graduated
  from the backlog for WS6.6.
- `quit` — ends the script and the app (`close()`, with the save-changes prompt bypassed since no
  one is present to answer it), so a scripted run doesn't need to be killed externally.

## What Remains Open

Candidate commands, not yet scoped:
- `run <computation>` — trigger an analysis/layout by name
- `open <file>`
- `wait-idle` — block until the GUI event loop is idle, for more precise timing than a fixed `delay`
- Basic control flow (`repeat N { ... }`) for stress-testing/averaging timing across runs
- Script-level variables/parameters passed from the command line, instead of everything hardcoded
  in the script text
- `cancel` — invoke `Graph::slotCancelComputation()` directly (same effect as clicking a
  `QProgressDialog`'s Cancel button, without needing one). Needs two prerequisites first, not just
  the command itself:
  - **A non-blocking dispatch variant.** Every command shipped so far — both dispatch patterns
    above — only advances the script *after* its own operation genuinely completes. So a script
    can never run `cancel` while a prior command is still in flight; the next line isn't
    dispatched until the previous one is already done.
  - **A way to trigger Information Centrality / Eigenvector Centrality from a script at all.**
    Neither `distances` nor `distances_bench` reaches `Matrix::inverse()`/`powerIteration()` — no
    existing command does.

  Motivating use case: verifying WS5 A5's cancellation-aware algebra kernels actually interrupt a
  real in-progress computation, not just accept the parameter without exercising it.

## Work Rules

Same as everywhere else: `./scripts/run_golden_compares.sh` clean before any commit. New commands
must stay dispatched through the real Qt event loop, not called directly, so scripted runs behave
the same as actual user interaction.
