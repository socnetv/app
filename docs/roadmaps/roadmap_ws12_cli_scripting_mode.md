# CLI Interactive/Scripting Mode (WS12)

## Goal

Drive SocNetV from the command line without manual clicking — profiling with `sample`/`perf`,
exercising GUI-triggered flows the headless `socnetv-cli` tool can't reach (it doesn't build the
GUI at all), automated demos/regression scenarios, and eventually external programs driving
SocNetV as a live component.

## Status

🚧 In progress. Eighteen commands shipped across #261/#262/WS14/WS6.6/this pass (see "Commands"
below); every command now logs a uniform `BENCH` line on completion. Eventually most of SocNetV's
functions should be reachable through interactive mode — see "Long-term direction" below for where
this is headed.

## CLI flags

- `--encoding <name>` — loads the startup file with a given text codec, bypassing the "Preview
  file & Choose Encoding" dialog.
- `--interactive-script <path>` — runs a plain-text script after startup, one command per line.
  Implementation in `MainWindow::runInteractiveScript()`/`processNextInteractiveCommand()`
  (`mainwindow.cpp`), which every command goes through the real Qt event loop and `graphThread`
  dispatch to reach, same as an actual user action — not a direct call. Two dispatch shapes are
  used, both documented in the Doxygen comment on `processNextInteractiveCommand()` itself:
  - **Single-step**: `QMetaObject::invokeMethod(activeGraph, lambda, Qt::QueuedConnection)` queues
    a lambda onto `activeGraph`'s own thread and returns immediately, without waiting for it to
    finish — so timing/`BENCH` logging for these commands happens *inside* that lambda, at actual
    completion, not around the `invokeMethod` call itself.
  - **Two-step** (`distances`/`distances_bench`, anything long enough to want a progress dialog):
    `runGraphOperationAsync(operation, waitMessage, onComplete)` — one lambda does the (possibly
    slow) work, a second runs only once that's genuinely finished, to log `BENCH` and advance the
    script. Both lambdas share timer/result state via `std::shared_ptr`, since a plain local
    variable wouldn't survive between two separate lambdas.

## Output format

Every command logs exactly one line on completion:
```
BENCH <command> [command-specific fields] N=<node count> E=<edge count> elapsed_ms=<N>
```
via `qInfo()` — deliberately not `qDebug()`/`qCDebug()`, so these lines keep printing regardless of
logging-category filter state (quiet-by-default, `-d` flags, etc.). This is uniform across every
command below, not just the ones originally added for benchmarking.

## Commands

- `delay X` — wait X seconds before the next command. `elapsed_ms` in its `BENCH` line should read
  ~= the requested delay — a cheap sanity check that scripted delays aren't drifting under load.
- `new` — File → New.
- `relation N` — switch to relation N.
- `unilateral` — toggle unilateral edges. Triggers the real `editFilterEdgesUnilateralAct`
  `QAction`, so it also exercises that action's own pre-existing direct-call blocking behavior
  (`MainWindow::slotEditFilterEdgesUnilateral()` calls `Graph::edgeFilterUnilateral()`
  synchronously across threads — a separate, not-yet-fixed issue, same family as #254).
- `erdos N p directed|undirected` — generates an Erdős–Rényi `G(n,p)` network.
- `save path` — saves the current network as GraphML.
- `add-node` — adds a node at a random position.
- `add-edge source target [weight]` — adds a directed edge (default weight 1).
- `add-relation name` — adds a new relation and switches to it.
- `distances [weights] [inverse] [dropisolates]` — mirrors the real Cohesion → Distances Matrix
  menu action (`slotAnalyzeMatrixDistances()`) exactly: same computation (`writeMatrix()` →
  `graphMatrixDistanceGeodesicCreate()`), same `runGraphOperationAsync()` dispatch, same output
  file — just without opening a `TextEditor` afterward, and without `askAboutEdgeWeights()`'s modal
  prompt (the tokens answer what it would ask). Trailing tokens are order-independent; presence of
  a token means true, absence means false. Previously called `graphDistancesGeodesic()` directly
  and crashed on some networks (`DistanceEngine::initRun` → `Graph::isSymmetric` → `edgeExists` →
  `GraphVertex::hasEdgeTo`, invalid `QMultiHash` access) — the real menu action, computing via
  `writeMatrix()`, did not crash on the same network, so this command now goes through that path
  instead.
- `distances_bench [weights] [inverse] [dropisolates] [centralities]` — benchmarking-only sibling
  of `distances`: same dispatch and computation, no disk write. `centralities` has no real-menu
  equivalent (the GUI computes each centrality index via ~9 separate menu actions, not one combined
  action), so it lives here rather than on `distances`.
- `render` — forces a synchronous `graphicsWidget->viewport()->repaint()` (unlike `update()`,
  which only schedules one). Added for WS6.6's canvas rendering-perf kernel
  (`roadmap_ws6_testing_ci_regression.md`).
- `bulk-node-size <N>` — calls `slotEditNodeSizeAll(N)` directly (nonzero `N` skips its modal
  `QInputDialog`).
- `bulk-edge-color <name>` — calls `slotEditEdgeColorAll(QColor(name))` directly (a valid color
  skips its modal `QColorDialog`).
- `move <node> <x> <y>` — sets an absolute canvas position via `Graph::vertexPosSet()`. Graduated
  from the backlog below for WS6.6.
- `quit` — ends the script and the app (`close()`, with the save-changes prompt bypassed since no
  one is present to answer it), so a scripted run doesn't need to be killed externally.

## Backlog

Candidate commands, not yet scoped:
- `run <computation>` — trigger an analysis/layout by name
- `open <file>`
- `wait-idle` — block until the GUI event loop is idle, for more precise timing than a fixed `delay`
- Basic control flow (`repeat N { ... }`) for stress-testing/averaging timing across runs
- Script-level variables/parameters passed from the command line, instead of everything hardcoded
  in the script text

## Long-term direction

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

## Work Rules

Same as everywhere else: `./scripts/run_golden_compares.sh` clean before any commit. New commands
must stay dispatched through the real Qt event loop, not called directly, so scripted runs behave
the same as actual user interaction.
