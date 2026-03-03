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

## Milestones
- P1 (DONE): Document `Parser` entrypoints + signal contract (this section).
- P2: Headless “parse-only” mode (no UI assumptions), shared between GUI/CLI via a single wiring helper.
- P3: Replace signal fan-out with an explicit builder / transaction API (still semantics-identical).
- P4: Golden parse tests for key formats (GraphML + at least one other), plus roundtrip stability tests.

## Work Rules
- No semantic changes to file parsing behavior during extraction.
- Always test with known datasets and inspect golden output diffs.
- After structural changes:
  - build (GUI + CLI)
  - `./scripts/run_golden_compares.sh`
  - `./scripts/run_benchmarks.sh` (includes IO benchmarks)