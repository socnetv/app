# IO / Parser Refactor Roadmap (Skeleton)

## Goal
Reduce tight coupling between `Parser` (Qt signals/threads) and core graph/model state.

## Current Reality
- `Parser` emits signals that `Graph` consumes to mutate state.
- Some headless loading now exists via CLI harness wiring.

## Target Direction
- Introduce an IO layer that can load into a model deterministically.
- Keep Qt signal-based pipeline initially, but provide a non-UI entry point.

## Milestones
- P1: Document Parser API and signal contract
- P2: Headless “parse-only” mode (no UI assumptions)
- P3: Replace signal fan-out with an explicit builder / transaction API
- P4: Golden parse tests for key formats (GraphML + at least one other)

## Work Rules
- No semantic changes to file parsing behavior during extraction.
- Always test with known datasets.
