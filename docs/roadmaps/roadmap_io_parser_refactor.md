# IO / Parser Refactor Roadmap

## Goal
Reduce tight coupling between `Parser` (Qt signals/threads) and core `Graph` state, while preserving **identical parsing semantics** and **deterministic outputs** (golden parity required).

## Scope & Non-Goals
- ✅ Mechanical extraction / boundary cleanup
- ✅ Deterministic, non-UI entrypoints for loading
- ❌ No changes to parsing behavior, graph semantics, or numeric outputs
- ❌ No UI object construction in IO/algorithm slices (F4 boundary)

## Current Reality
- `Parser` emits signals; `Graph` consumes them to mutate graph/model state.
- Headless loading exists via CLI wiring (local `Parser` on a local `QThread`, signals connected directly to `Graph` mutators).
- UI path uses similar signal wiring (must remain behavior-identical).

## Parser → Graph Signal Contract (P1)
The following signals define the mutation stream from `Parser` into `Graph`.
This mapping must remain stable during WS4:

### Relation lifecycle
- `Parser::signalAddNewRelation(QString relName, bool changeRelation)`
  → `Graph::relationAdd(QString relName, bool changeRelation)`
- `Parser::signalSetRelation(int relIndex, bool updateUI)`
  → `Graph::relationSet(int relIndex, bool updateUI)`

### Node creation
- `Parser::signalCreateNode(...)`
  → `Graph::vertexCreate(...)`
- `Parser::signalCreateNodeAtPosRandom(bool signalMW)`
  → `Graph::vertexCreateAtPosRandom(bool signalMW)`
- `Parser::signalCreateNodeAtPosRandomWithLabel(int num, QString label, bool signalMW)`
  → `Graph::vertexCreateAtPosRandomWithLabel(int num, QString label, bool signalMW)`

### Edge creation
- `Parser::signalCreateEdge(int source, int target, double weight, QString color, int edgeDirType, bool arrows, bool bezier, QString edgeLabel, bool signalMW)`
  → `Graph::edgeCreate(...)`

### File loaded / finalization
- `Parser::signalFileLoaded(int fileType, QString fileName, QString netName, int totalNodes, int totalLinks, int edgeDirType, qint64 elapsedTime, QString message)`
  → `Graph::graphFileLoaded(...)`

### Legacy cleanup
- `Parser::removeDummyNode(int)`
  → `Graph::vertexRemoveDummyNode(int)`

### Terminal signal
- `Parser::finished(QString)`
  Used by headless loaders as a fallback “done” signal to avoid deadlocks if `Graph::signalGraphLoaded` does not fire.

### Completion condition (important)
Headless loaders should block until `Graph::signalGraphLoaded` is emitted (preferred completion),
with `Parser::finished` used only as a safety fallback.

## Target Direction
Introduce an IO layer that can load deterministically into a model.
Keep the Qt signal-based pipeline initially, but provide a non-UI entry point.
Status: Parser signal fan-out is now defined in a single location.
GUI and CLI loading paths are behaviorally aligned.

## Milestones

### P1 (DONE): Document `Parser` entrypoints + signal contract 
### P2 (DONE): Extracted centralized Parser→Graph wiring helper
  (`src/graph/io/graph_parser_wiring.{h,cpp}`) and updated both
  GUI (`Graph::loadFile`) and CLI headless loader to use it.
  No semantic changes. Golden + benchmark parity verified.
### P3: Replace signal fan-out with an explicit builder / transaction API (still semantics-identical).
**Goal:** Keep parsing semantics identical, but stop treating signals as “the API”.
Parser should write into a narrow “mutation sink” (builder/transaction) that is explicit and testable. Qt signals may remain as a compatibility layer temporarily, but they stop being the primary data plane.

#### P3.0 — Parser audit + invariants snapshot (NO CODE MOVES YET)

**Deliverables**

* List all Parser entrypoints used by GUI + CLI (public methods, slots, helpers).
* Identify where Parser currently:

  * decides directed/undirected defaults
  * decides weighted/unweighted defaults
  * increments counters (`totalNodes`, `totalLinks`, relations count)
  * emits “final” signals (`signalFileLoaded`, `finished`)
* Document thread assumptions: what must run on parser thread vs graph thread.

**Verification**

* No build changes required.
* Run:

  * `./scripts/run_golden_compares.sh`
  * `./scripts/run_benchmarks.sh`

#### P3.1 — Introduce “mutation sink” interface (new, minimal)

**Deliverables**

* New interface (header-only initially is OK), e.g. `IGraphParseSink` or `GraphParseTransaction`.
* Methods map 1:1 to current mutation operations (no new semantics):

  * begin/end relation, set relation
  * create node variants
  * create edge
  * optional “remove dummy node”
  * finalize (fileType, edgeDirType, etc.) **but only as data**, not as UI signals

**Rules**

* No UI types.
* No ownership/lifetime surprises.
* Keep everything synchronous at first.

**Verification**

* Build GUI + CLI.
* Goldens + benchmarks.

#### P3.2 — Implement Graph-backed sink adapter (thin wrapper)

**Deliverables**

* Implement sink that calls Graph methods **with the exact same arguments** currently provided via signals.
* This is effectively the “explicit equivalent” of `wireParserToGraph()`.

**Verification**

* Build GUI + CLI.
* Goldens + benchmarks.

#### P3.3 — Parser writes to sink (keep signals as compatibility, initially)

**Deliverables**

* Parser gains an optional sink pointer/reference.
* Parser internals call `sink->createEdge(...)` etc.
* Keep emitting signals too for now (or gate via a flag), but ensure **ordering is identical**.

**Verification**

* Build GUI + CLI.
* Goldens + benchmarks.

#### P3.4 — Swap GUI path to sink-first (signals only for UI notifications)

**Deliverables**

* `Graph::loadFile()` (GUI path) creates the sink and passes it to Parser.
* Parser→Graph mutation happens via sink, not via Qt connects.
* Keep `signalFileLoaded`/`finished` only as completion notifications (or bridge them).

**Verification**

* Build GUI + CLI.
* Goldens + benchmarks.

#### P3.5 — Swap CLI headless path to sink-first (remove dependence on signal wiring)

**Deliverables**

* `loadGraphHeadless()` passes a sink to Parser.
* Completion still waits on `Graph::signalGraphLoaded` (preferred), `Parser::finished` as fallback.
* Remove any now-unused wiring in CLI loader.

**Verification**

* Build CLI.
* Goldens + benchmarks (especially IO benchmarks).

#### P3.6 — Deprecate signal wiring helper (keep temporarily, then delete)

**Deliverables**

* Mark `wireParserToGraph()` as transitional / deprecated in comments.
* Once both GUI + CLI are sink-based and stable, remove the helper and the obsolete connects.

**Verification**

* Build GUI + CLI.
* Goldens + benchmarks.

#### P3.7 — Document final contract + add targeted parse goldens (handoff to P4)

**Deliverables**

* Update the “Parser contract” section to describe sink API as canonical.
* Add/confirm parse/IO baselines (P4) for at least:

  * GraphML
  * Pajek
  * one “weird” format (DL or DOT) depending on current stability

**Verification**

* Goldens + benchmarks.

**Work Discipline**

* One sub-step per commit.
* After every commit: build, run goldens, run benchmarks, inspect diffs.

### P4: Golden parse tests for key formats (GraphML + at least one other), plus roundtrip stability tests.

---


## Work Rules
- No semantic changes to file parsing behavior during extraction.
- Always test with known datasets and inspect golden output diffs.
- After structural changes:
  - build (GUI + CLI)
  - `./scripts/run_golden_compares.sh`
  - `./scripts/run_benchmarks.sh` (includes IO benchmarks)