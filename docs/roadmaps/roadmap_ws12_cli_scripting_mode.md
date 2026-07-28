# CLI Interactive/Scripting Mode (WS12)

## Status

**First step shipped (#261).** Everything past that is backlog (#262) — not scoped in detail yet.
This file is intentionally minimal; flesh out once there's a concrete next command to implement.

## Goal

Drive SocNetV from the command line without manual clicking, so behavior (including performance)
can be scripted and reproduced exactly — profiling with `sample`/`perf`, exercising GUI-triggered
flows the headless `socnetv-cli` tool can't reach (it doesn't build the GUI at all), and eventually
automated demos/regression scenarios that need real GUI interaction.

## Shipped (#261)

- `--encoding <name>` — loads the startup file with a given text codec, bypassing the "Preview
  file & Choose Encoding" dialog.
- `--interactive-script <path>` — runs a plain-text script after startup, one command per line.
  Commands so far: `delay X` (wait X seconds), `new` (File → New). Implementation in
  `MainWindow::runInteractiveScript()`/`processNextInteractiveCommand()` (`mainwindow.cpp`),
  dispatched via `QTimer::singleShot` so each command goes through the real Qt event loop.
- Was the tool that made it possible to actually root-cause #260 — reproducible, unattended repro
  of "load a large network, then clear it" for live `sample` profiling.

## Backlog (#262)

Candidate commands, not yet scoped:
- `move <node> <x> <y>` — move a node programmatically
- `run <computation>` — trigger an analysis/layout by name
- `open <file>` / `save <file>`
- `wait-idle` — block until the GUI event loop is idle, for more precise timing than a fixed `delay`
- Basic control flow (`repeat N { ... }`) for stress-testing/averaging timing across runs
- Script-level variables/parameters passed from the command line, instead of everything hardcoded
  in the script text

## Work Rules

Same as everywhere else: `./scripts/run_golden_compares.sh` clean before any commit. New commands
should stay dispatched through the real Qt event loop (`QTimer::singleShot`), not called directly,
so scripted runs behave the same as actual user interaction — this is what made #261 useful for
profiling #260 in the first place.
