# IO / Parser Refactor Roadmap

## Goal
Reduce tight coupling between `Parser` (Qt signals/threads) and core `Graph` state, while preserving **identical parsing semantics** and **deterministic outputs** (golden parity required).

## Scope & Non-Goals
- ✅ Mechanical extraction / boundary cleanup
- ✅ Deterministic, non-UI entrypoints for loading
- ❌ No changes to parsing behavior, graph semantics, or numeric outputs
- ❌ No UI object construction in IO/algorithm slices (F4 boundary)

## istorical Context Before WS4

* `Parser` mutated `Graph` via Qt signals.
* GUI and CLI wired those signals to `Graph` mutators.
* Mutation contract was implicit and spread across multiple locations.
* Signal fan-out was the de facto API.

This was fragile and hard to test deterministically.

## Current Reality

- `Parser` now mutates `Graph` exclusively via `IGraphParseSink`.
- Legacy Parser→Graph signal fan-out has been removed.
- Headless and GUI loading paths are sink-backed and behaviorally identical.
- Parsing still runs on a dedicated worker thread (GUI + CLI).

## Parser → Graph Mutation Contract (Post-P3)

The mutation stream from `Parser` into `Graph` is now defined by `IGraphParseSink`.

All graph/model mutations during parsing must flow exclusively through:

- `addNewRelation(...)`
- `setRelation(...)`
- `createNode(...)`
- `createNodeAtPosRandom(...)`
- `createNodeAtPosRandomWithLabel(...)`
- `createEdge(...)`
- `removeDummyNode(...)`
- `fileLoaded(...)`

Legacy Qt mutation signals have been removed (P3.6–P3.7).

### Terminal signal

- `Parser::finished(QString)` remains for lifecycle completion.
- Preferred completion signal remains `Graph::signalGraphLoaded`.

### Completion condition (important)

Headless loaders block until:
- Preferred: `Graph::signalGraphLoaded`
- Fallback: `Parser::finished`


## Milestones

### P1 — Parser API + signal/slot contract documented ✅ DONE
- Documented Parser mutation surface and completion conditions.
- Historical note: completion condition = `Graph::signalGraphLoaded`, with `Parser::finished` as fallback.

### P2 — Centralize Parser→Graph wiring helper ✅ DONE (superseded)
- Implemented shared wiring helper to prevent drift (GUI + headless).
- NOTE: This helper was later removed once sink-based mutation became the sole plane (see P3.5).

### P3 — Introduce explicit sink-based mutation plane ✅ DONE

#### P3.1 — Add mutation sink interface ✅ DONE
- Added `IGraphParseSink` (signal-parity contract).

#### P3.2 — Add Graph-backed sink adapter ✅ DONE
- Added `GraphParseSinkGraph` forwarding sink → Graph façade mutators.

#### P3.3 — Parser supports optional sink ✅ DONE
- Added `Parser::setParseSink(...)` and `Parser::setOwnedParseSink(...)` for thread-safe lifetime.
- Parser forwards mutation events to sink (preserving ordering and payload parity).

#### P3.4 — Switch GUI load path to sink-backed mutation ✅ DONE
- `Graph::loadFile(...)` configures an owned `GraphParseSinkGraph` on the Parser.
- GUI no longer uses mutation signal wiring.

#### P3.5 — Remove legacy Parser→Graph wiring helper ✅ DONE
- Deleted `src/graph/io/graph_parser_wiring.{h,cpp}` and removed from build files.

#### P3.6 — Stop emitting legacy Parser mutation signals ✅ DONE
- Removed `emit` for mutation signals (sink is the sole mutation plane).

#### P3.7 — Remove legacy Parser mutation signals ✅ DONE
- Deleted unused mutation signal declarations from `parser.h`.
- Parsing mutations now flow exclusively through `IGraphParseSink` (GUI + headless).

### P4 — Golden IO coverage + roundtrip stability ✅ DONE
- Implemented via CLI golden harness:
  - `scripts/run_golden_compares.sh`
  - `io_roundtrip` kernel baselines in `src/tools/baselines/io_roundtrip/`
- Coverage includes GraphML (FT1) plus multiple additional formats (e.g., Pajek/Adj/DOT/DL/GML/EdgeList).
- Formats without exporters are still covered: “export skipped” outcome is baseline-locked.


Notes:
- Adding new IO coverage is done by:
  1) generating a new baseline JSON via `socnetv-cli --kernel io_roundtrip ... --dump-json ...`
  2) committing it
  3) adding a `run_case_io` entry to `run_golden_compares.sh`

---


## Next Steps

### P5 — Introduce ParseConfig boundary (reduce coupling) ⏳ TODO

Goal:
Eliminate implicit coupling between Parser and Graph/UI defaults while preserving exact parsing semantics and numeric outputs.

Current issue:
`Parser::load(...)` accepts a large parameter list (node defaults, edge defaults, canvas size, format flags) and threads them through format handlers.

Plan:
- Introduce an immutable `ParseConfig` struct (QtCore-only types).
- At the start of `Parser::load(...)`, construct `ParseConfig cfg{...}` from existing parameters.
- Refactor internal parse handlers to accept `const ParseConfig&` instead of individual defaults.
- Do NOT change the public signature of `Parser::load(...)` initially.

Rules:
- No logic changes.
- No reordering of operations.
- No change to emission order or values passed to sink.
- Golden parity required.

Verification:
- build
- `./scripts/run_golden_compares.sh`
- `./scripts/run_benchmarks.sh`

### P6 — Split parser.cpp by file type translation units ⏳ TODO

Goal:
Reduce translation unit size and improve locality without changing behavior.

Target layout (all remain `Parser::` member functions):

- `src/parser/parser_common.cpp`      (shared helpers)
- `src/parser/parser_edgelist.cpp`    (Simple + Weighted edgelists)
- `src/parser/parser_adjacency.cpp`   (Adjacency / Sociomatrix)
- `src/parser/parser_dl.cpp`          (UCINET DL)
- `src/parser/parser_pajek.cpp`       (Pajek NET/PAJ)
- `src/parser/parser_graphml.cpp`     (GraphML XML)
- `src/parser/parser_gml.cpp`         (GML)
- `src/parser/parser_dot.cpp`         (GraphViz DOT)

Execution discipline:
- Extract one format per commit.
- Each commit must:
  - compile (GUI + CLI)
  - pass goldens
  - pass benchmarks
- No logic edits during extraction.
- Only mechanical move + include fixes.

Build updates required:
- Add each new `.cpp` to:
  - CMake target sources
  - `socnetv.pro`

Code size policy:
- Preferred TU size: 500–1500 LOC
- >3000 LOC after extraction → candidate for further slicing

### P7 — Optional: explicit parse transaction layer ⏳ OPTIONAL

Goal:
Make parsing explicitly transactional while preserving semantics.

Idea:
- Introduce a lightweight `ParseTransaction` object created in `Parser::load(...)`.
- Parser writes events into the transaction.
- Sink applies mutations.
- No change in mutation ordering or values.

Proceed only if it reduces complexity without altering behavior.

---

## Current Architectural State (Post-P3)

- Parser mutation plane is sink-only (`IGraphParseSink`).
- No Parser→Graph mutation signals remain.
- GUI and CLI loading paths are unified.
- Golden regression harness enforces IO + roundtrip stability.
- Benchmarks enforce performance guardrails.