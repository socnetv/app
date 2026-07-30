# CLI Interactive/Scripting Mode (WS12)

## Status

First step shipped (#261), expanded with seven more commands (#262). Eventually most of
SocNetV's functions should be reachable through interactive mode — see "Long-term direction"
below for where this is headed.

## Goal

Drive SocNetV from the command line without manual clicking — profiling with `sample`/`perf`,
exercising GUI-triggered flows the headless `socnetv-cli` tool can't reach (it doesn't build the
GUI at all), automated demos/regression scenarios, and eventually external programs driving
SocNetV as a live component.

## CLI flags

- `--encoding <name>` — loads the startup file with a given text codec, bypassing the "Preview
  file & Choose Encoding" dialog.
- `--interactive-script <path>` — runs a plain-text script after startup, one command per line.
  Implementation in `MainWindow::runInteractiveScript()`/`processNextInteractiveCommand()`
  (`mainwindow.cpp`), dispatched via `QTimer::singleShot`/`QMetaObject::invokeMethod(...,
  Qt::QueuedConnection)` so each command goes through the real Qt event loop and `graphThread`
  dispatch, same as an actual user action — not a direct call.

## Commands

- `delay X` — wait X seconds before the next command.
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
- `distances` — computes geodesic distances via `Graph::graphDistancesGeodesic(false, ...)`,
  bypassing `slotAnalyzeMatrixDistances()`'s report-writing/HTML-viewer path. Logs one `qInfo()`
  line (`BENCH distances ... elapsed_ms=...`) — deliberately `qInfo()`, not `qDebug()`/`qCDebug()`,
  so it keeps printing regardless of any logging-category filter state. Added for WS14
  logging-cost before/after measurement (see `roadmap_ws14_logging_cost.md`).
- `distances centralities` — same, with `computeCentralities=true`.

## Backlog

Candidate commands, not yet scoped:
- `move <node> <x> <y>` — move a node programmatically
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
