# SocNetV Interactive Script Mode

`--interactive-script <path>` runs the real, full `socnetv` GUI application (event loop, `Graph`,
`GraphicsWidget`, everything) driven by a plain-text script instead of manual clicking.

---

# Purpose

Scripted demos, automated regression/exploratory testing, and reproducible profiling
(`sample`/`perf`) of GUI-triggered flows that the headless `socnetv-cli` tool can't reach — it
doesn't build the GUI at all. See [`SOCNETV_CLI_REGRESSION_TOOL.md`](SOCNETV_CLI_REGRESSION_TOOL.md)
for that sibling, headless tool; use interactive-script mode instead whenever the scenario touches
`GraphicsWidget`, menu actions, dialogs, or anything else that only exists on the GUI thread.

---

# Usage

```bash
socnetv --interactive-script path/to/script.txt
```

The script is a plain-text file, one command per line. Blank lines and `#`-prefixed comment lines
are ignored. Every command logs exactly one line to stdout on completion:

```
BENCH <command> [command-specific fields] N=<node count> E=<edge count> elapsed_ms=<N>
```

via `qInfo()`, so these lines print regardless of logging-category filter state.

Commands run in order; each command's own work genuinely finishes (not just gets queued) before
the next command starts.

---

# Commands

## Control

| Command | Effect |
|---|---|
| `delay X` | Wait X seconds before the next command. |
| `new` | File → New. |
| `quit` | Ends the script and the app (bypasses the save-changes prompt). |

## Network construction

| Command | Effect |
|---|---|
| `relation N` | Switch to relation N. |
| `add-relation name` | Add a new relation and switch to it. |
| `add-node` | Add a node at a random position. |
| `add-edge source target [weight]` | Add a directed edge (default weight 1). |
| `erdos N p directed\|undirected` | Generate an Erdős–Rényi `G(n,p)` network. |
| `move <node> <x> <y>` | Set an absolute canvas position. |
| `save path` | Save the current network as GraphML. |

## Filters

| Command | Effect |
|---|---|
| `unilateral` | Toggle unilateral edges. |
| `filter-ego` | Ego-network filter. |
| `filter-isolates` | Filter isolated nodes. |
| `symmetrize-strongties` | Symmetrize by strong ties. |
| `symmetrize-cocitation` | Symmetrize by cocitation. |

## Distances / matrices

| Command | Effect |
|---|---|
| `distances [weights] [inverse] [dropisolates] [csv]` | Distances Matrix report — mirrors Cohesion → Distances Matrix. |
| `distances-bench [weights] [inverse] [dropisolates] [centralities]` | Same computation, no disk write; benchmarking only. |

## Centrality / prestige reports

Each mirrors its real `Analyze` menu action exactly:

```
report-centrality-degree [weights] [dropisolates] [csv]
report-centrality-degree-signed [weights] [dropisolates] [csv]
report-centrality-pn [all|out|in] [dropisolates] [csv]
report-centrality-closeness [weights] [inverse] [dropisolates] [csv]
report-centrality-closeness-ir [weights] [inverse] [dropisolates] [csv]
report-centrality-betweenness [weights] [inverse] [dropisolates] [csv]
report-centrality-stress [weights] [inverse] [dropisolates] [csv]
report-centrality-eccentricity [weights] [inverse] [dropisolates] [csv]
report-centrality-power [weights] [inverse] [dropisolates] [csv]
report-centrality-information [weights] [inverse] [csv]
report-centrality-eigenvector [weights] [inverse] [csv]
report-prestige-degree [weights] [dropisolates] [csv]
report-prestige-proximity [dropisolates] [csv]
report-prestige-pagerank [dropisolates] [csv]
```

Notes:
- Trailing tokens are order-independent; presence means true, absence means false.
- `csv` selects CSV output explicitly (a script has no Settings dialog to read a persisted
  preference from).
- `report-centrality-information` and `report-centrality-eigenvector` have no `dropisolates`
  token. `report-prestige-proximity` and `report-prestige-pagerank` have no `weights`/`inverse`
  tokens (the underlying measures don't take them).
- `report-centrality-pn`'s mode is a required 3-way choice (`all`/`out`/`in`, default `all`), the
  one command here using a bare positional token instead of a boolean flag — PN centrality has no
  `weights` token either (it considers tie sign only).

## Other reports

```
report-reciprocity [weights] [csv]
report-eccentricity [weights] [inverse] [dropisolates] [csv]
report-clustering-coefficient [csv]
report-triad-census [csv]
```

`report-clustering-coefficient` and `report-triad-census` have no `weights` token (both fix
`considerWeights = true`).

## Canvas / bulk edit

| Command | Effect |
|---|---|
| `render` | Force a synchronous canvas repaint. |
| `bulk-node-size <N>` | Set all node sizes to N. |
| `bulk-edge-color <name>` | Set all edge colors. |

---

# Command naming convention

New commands are named and shaped after the equivalent operation's name in established,
widely-used SNA scripting ecosystems, wherever a clear equivalent exists — this keeps
cross-checking SocNetV's own computations against an outside implementation straightforward, and
lowers friction porting an existing script. SocNetV's own existing conventions in this file take
precedence where they'd conflict; this is a preference for new commands, not a mandate to rename
existing ones.

---

# Example

```
# example.txt
new
erdos 50 0.1 undirected
relation 1
report-centrality-betweenness weights csv
render
save /tmp/demo.graphml
quit
```

```bash
socnetv --interactive-script example.txt
```

---

# Extending

Adding a new command means adding a branch to
`MainWindow::processNextInteractiveCommand()` (`mainwindow.cpp`). Three dispatch shapes exist,
documented in that function's own Doxygen comment — get this wrong and a later command in the
script can race ahead of an earlier one's still-in-flight work:

- **No dispatch** — a direct, blocking call on the GUI thread (e.g. `new`, `render`): correct only
  when the call is genuinely synchronous.
- **Single-step** — `QMetaObject::invokeMethod(activeGraph, lambda, Qt::QueuedConnection)`:
  `BENCH` logging and advancing to the next command must both happen *inside* the lambda, at
  actual completion — never around the `invokeMethod` call itself.
- **Two-step** — `runGraphOperationAsync(operation, waitMessage, onComplete)` for anything long
  enough to want a progress dialog: a second lambda runs only once the first is genuinely
  finished, to log `BENCH` and advance the script.

New commands must stay dispatched through the real Qt event loop, not called directly, so scripted
runs behave the same as actual user interaction. Development history, open design questions
(streaming/monitor mode, `cancel`, control flow), and known issues live in
[`roadmap_ws12_cli_scripting_mode.md`](roadmaps/roadmap_ws12_cli_scripting_mode.md) — this file is
the current-state reference, not the design log.
