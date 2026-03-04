# IO / Parser Refactor Roadmap

## Goal

Reduce tight coupling between `Parser` (Qt signals/threads) and core `Graph` state, while preserving **identical parsing semantics** and **deterministic outputs** (golden parity required).

## Scope & Non-Goals

* ✅ Mechanical extraction / boundary cleanup
* ✅ Deterministic, non-UI entrypoints for loading
* ❌ No changes to parsing behavior, graph semantics, or numeric outputs
* ❌ No UI object construction in IO/algorithm slices (F4 boundary)

---

# Historical Context Before WS4

* `Parser` mutated `Graph` via Qt signals.
* GUI and CLI wired those signals to `Graph` mutators.
* Mutation contract was implicit and spread across multiple locations.
* Signal fan-out was the de facto API.

This was fragile and hard to test deterministically.

---

# Current Reality

* `Parser` now mutates `Graph` exclusively via `IGraphParseSink`.
* Legacy Parser→Graph signal fan-out has been removed.
* Headless and GUI loading paths are sink-backed and behaviorally identical.
* Parsing still runs on a dedicated worker thread (GUI + CLI).

---

# Parser → Graph Mutation Contract (Post-P3)

The mutation stream from `Parser` into `Graph` is now defined by `IGraphParseSink`.

All graph/model mutations during parsing must flow exclusively through:

* `addNewRelation(...)`
* `setRelation(...)`
* `createNode(...)`
* `createNodeAtPosRandom(...)`
* `createNodeAtPosRandomWithLabel(...)`
* `createEdge(...)`
* `removeDummyNode(...)`
* `fileLoaded(...)`

Legacy Qt mutation signals have been removed (P3.6–P3.7).

### Terminal signal

* `Parser::finished(QString)` remains for lifecycle completion.
* Preferred completion signal remains `Graph::signalGraphLoaded`.

### Completion condition (important)

Headless loaders block until:

* Preferred: `Graph::signalGraphLoaded`
* Fallback: `Parser::finished`

---

# Milestones

## P1 — Parser API + signal/slot contract documented ✅ DONE

* Documented Parser mutation surface and completion conditions.
* Historical note: completion condition = `Graph::signalGraphLoaded`, with `Parser::finished` as fallback.

---

## P2 — Centralize Parser→Graph wiring helper ✅ DONE (superseded)

* Implemented shared wiring helper to prevent drift (GUI + headless).

NOTE: This helper was later removed once sink-based mutation became the sole plane (see P3.5).

---

## P3 — Introduce explicit sink-based mutation plane ✅ DONE

### P3.1 — Add mutation sink interface ✅ DONE

* Added `IGraphParseSink` (signal-parity contract).

### P3.2 — Add Graph-backed sink adapter ✅ DONE

* Added `GraphParseSinkGraph` forwarding sink → Graph façade mutators.

### P3.3 — Parser supports optional sink ✅ DONE

* Added `Parser::setParseSink(...)` and `Parser::setOwnedParseSink(...)` for thread-safe lifetime.
* Parser forwards mutation events to sink (preserving ordering and payload parity).

### P3.4 — Switch GUI load path to sink-backed mutation ✅ DONE

* `Graph::loadFile(...)` configures an owned `GraphParseSinkGraph` on the Parser.
* GUI no longer uses mutation signal wiring.

### P3.5 — Remove legacy Parser→Graph wiring helper ✅ DONE

* Deleted `src/graph/io/graph_parser_wiring.{h,cpp}` and removed from build files.

### P3.6 — Stop emitting legacy Parser mutation signals ✅ DONE

* Removed `emit` for mutation signals (sink is the sole mutation plane).

### P3.7 — Remove legacy Parser mutation signals ✅ DONE

* Deleted unused mutation signal declarations from `parser.h`.
* Parsing mutations now flow exclusively through `IGraphParseSink` (GUI + headless).

---

## P4 — Golden IO coverage + roundtrip stability ✅ DONE

Implemented via CLI golden harness:

* `scripts/run_golden_compares.sh`
* `io_roundtrip` kernel baselines in `src/tools/baselines/io_roundtrip/`

Coverage includes:

* GraphML
* Pajek
* Adjacency
* DOT
* DL
* GML
* EdgeList

Formats without exporters are still covered: **“export skipped”** outcome is baseline-locked.

### Notes

Adding new IO coverage:

1. generate baseline JSON via
   `socnetv-cli --kernel io_roundtrip ... --dump-json ...`
2. commit baseline
3. add `run_case_io` entry in `run_golden_compares.sh`

---

# P5 — Introduce ParseConfig boundary ✅ DONE

Goal achieved:

Reduce coupling between `Parser::load(...)` and internal parse handlers while preserving semantics.

Implementation:

* Introduced immutable:

```
struct ParseConfig
```

* Constructed at the start of `Parser::load(...)` from existing parameters.
* Internal parse handlers progressively migrated to accept:

```
const ParseConfig&
```

instead of individual defaults.

Important constraints preserved:

* No logic changes
* No ordering changes
* No mutation stream changes
* No sink payload changes

Verification performed during migration:

* build
* `run_golden_compares.sh`
* `run_benchmarks.sh`

All outputs preserved.

---

# P6 — Split parser.cpp by file type translation units ✅ DONE

Goal:

Reduce translation unit size and improve locality without changing behavior.

### Execution

Performed incrementally, **one format per commit**, with:

* build verification
* golden regression tests
* benchmark checks

### Final layout

```
src/parser/
  parser_common.cpp
  parser_edgelist.cpp
  parser_adjacency.cpp
  parser_dl.cpp
  parser_pajek.cpp
  parser_graphml.cpp
  parser_gml.cpp
  parser_dot.cpp
  parser.cpp
```

### Responsibilities

**parser.cpp**

* Parser constructor / destructor
* sink wiring
* `load()` dispatcher
* `parseAsTwoModeSociomatrix`
* minimal coordinator logic

**parser_common.cpp**

* shared helpers (`isComment`, future utilities)

**parser_* files**

* format-specific parsing logic

### Extracted formats

* EdgeList (simple + weighted)
* Adjacency / Sociomatrix
* UCINET DL
* Pajek
* GraphML
* GML
* GraphViz DOT

### Additional helper relocation

During extraction, format-local helpers were moved to their respective files:

Examples:

* DL → `createRandomNodes`
* Pajek → `normalizeQuotedIdentifier`
* Adjacency →
  `createEdgesForRow`
  `createNodeWithDefaults`
  `containsReservedKeywords`

Shared helpers moved to `parser_common.cpp`.

### Build updates

Each new translation unit added to:

* `CMakeLists.txt`
* `socnetv.pro`

### Code size policy result

`parser.cpp` reduced from **~5500 LOC → ~1200 LOC**.

All parser TUs now comply with the project guideline:

* preferred: 500–1500 LOC

---

# Current Architectural State (Post-WS4)

The parser architecture is now:

```
Graph (façade)
      ↓
   Parser
      ↓
IGraphParseSink
      ↓
Graph mutation layer
```

Internally:

```
Parser::load()
   ├─ parser_edgelist.cpp
   ├─ parser_adjacency.cpp
   ├─ parser_dl.cpp
   ├─ parser_dot.cpp
   ├─ parser_gml.cpp
   ├─ parser_pajek.cpp
   └─ parser_graphml.cpp
```

Key properties:

* deterministic mutation stream
* format-local code isolation
* CLI + GUI share identical parsing logic
* IO behavior locked by golden regression harness
* performance guarded by benchmark suite

---

# Future Work

## P7 — Optional: explicit parse transaction layer ⏳ OPTIONAL

Goal:

Make parsing explicitly transactional while preserving semantics.

Concept:

```
ParseTransaction
```

* Created inside `Parser::load(...)`
* Parser emits events into the transaction
* Sink applies mutations

Benefits:

* clearer mutation lifecycle
* easier debugging / instrumentation
* possible replay or validation

Proceed only if it simplifies the design without altering behavior.

