# IO / Parser Refactor Roadmap (WS4)

## Goal

Reduce tight coupling between `Parser` (Qt signals/threads) and core `Graph` state, while
preserving identical parsing semantics and deterministic outputs (golden parity required).

## Status

✅ Done (P1–P6). Current parser/IO architecture (mutation pipeline, file layout, completion
signals) lives in [`README_DEVELOPER_NOTES.md`](../README_DEVELOPER_NOTES.md)'s "Parsing and I/O"
section, not here. P7 is optional and not started.

## What WS4 Delivered

- **P1–P3 — Sink-based mutation plane** — `IGraphParseSink` introduced as the sole `Parser`→`Graph`
  mutation contract, replacing the previous ad-hoc Qt-signal mutation fan-out entirely (signals
  removed, not just superseded). GUI and headless CLI now share identical parsing behavior.
- **P4 — Golden IO coverage** — `io_roundtrip` kernel baselines for GraphML, Pajek, Adjacency, DOT,
  DL, GML, EdgeList; formats without an exporter still get an "export skipped" baseline-locked
  outcome.
- **P5 — `ParseConfig` boundary** — an immutable `ParseConfig` struct constructed once at the start
  of `Parser::load()`, replacing scattered individual-parameter defaults across internal parse
  handlers. No logic/ordering/mutation-stream changes.
- **P6 — Split `parser.cpp` by format** — one translation unit per format under `src/parser/`,
  done incrementally (one format per commit, golden + benchmark verified each time).
  `parser.cpp` itself: ~5,500 LOC → ~1,200 LOC.

## What Remains Open

- **P7 (optional)** — an explicit `ParseTransaction` layer wrapping the mutation stream, for a
  clearer lifecycle/easier instrumentation. Not started; proceed only if it simplifies the design
  without altering behavior — not required for anything currently blocked.
