# Undo/Redo (WS13)

## Status

Just created (2026-07-30), not started. Scoped at a high level only — detailed technical design
(per-mutation-type state capture, stack architecture) has not been done yet. See "Open questions"
below for what's still unresolved.

## Origin

#31 is a long-standing user request: *"My main frustration, as a beginner, is that I can't find an
undo button; when I make large-scale mistakes, I find myself having to start all over."* No technical
prerequisite is stated in the issue itself.

This was previously tracked as a cross-cutting dependency of
[`roadmap_ws3_architecture_performance.md`](roadmap_ws3_architecture_performance.md), gated on "at
least M2" of that roadmap under the assumption that structural undo needs a stable domain model
first. That assumption was investigated on 2026-07-30 and found false — see WS3's "History: the
domain-model assumption, and why it's gone" section for the full trace. Undo doesn't structurally
depend on anything WS3 was building. Spun out to its own workstream rather than folded back into WS3
(now narrowed to evidence-driven fixes, not a home for "things that need `Graph` to be cleaner
first") or into WS9 (marked complete/shipped; reopening it to add a substantial new feature was the
wrong container — same reasoning that led to spinning up WS11/WS12 as their own workstreams earlier).

## Existing precedent: filter/visibility undo (already shipped, via WS9)

`m_visibilityHistory` (`QStack<GraphVisibilitySnapshot>`, declared in `graph.h`, implemented in
`src/graph/filters/graph_node_filters.cpp`) is a real, working, shipped undo mechanism:

- Push a `GraphVisibilitySnapshot` onto the stack before every non-destructive filter operation.
- Pop and restore on "Restore All" or arbitrary-index chip removal (`Graph::vertexFilterRemoveAt()`
  drains the stack, restores the pre-filter base, and replays all remaining snapshots' `FilterSpec`).
- Plain value-type snapshots — zero `QUndoStack`, zero Qt command-pattern machinery.

Covers today: centrality filter, ego network filter, selection filter, edge weight filter, attribute
filter, and the query builder (#221). This is real evidence that undo is achievable against `Graph`'s
existing (unrestructured) mutation API — the starting point for WS13, not a pattern to invent fresh.

## What's not covered today — the actual gap #31 is about

- **Structural mutations**: `vertexCreate`/`vertexRemove`, `edgeCreate`/`edgeRemove`, relation
  add/remove. Unlike a visibility toggle, reversing these requires capturing real state before the
  mutation (a removed vertex's full edge list, attributes, position — not just an on/off flag).
- **Attribute/property edits**: label, size, color, shape, weight, custom attributes.
- **Bulk edits** (WS9 Feature 3 Phase 5) explicitly bypass any undo mechanism today — noted as a
  known gap in `roadmap_ws9_graph_exploration.md` at the time it shipped.

## Open questions — not yet scoped

- Does the snapshot-stack pattern extend cleanly to structural mutations, or does per-mutation
  state-capture cost (copying full vertex/edge state) make it impractical for bulk operations (large
  random-network generation, subgraph deletion, multi-row bulk edit)?
- One shared stack for structural + attribute + visibility undo, or separate stacks (mirroring the
  existing node/edge visibility split)?
- Interaction with WS3 M2's batched edge-visibility signal path (`EdgeVisibilityChange` batching for
  relation switches/unilateral toggles) — does a batched operation need one snapshot per batch, or
  one per underlying edge change?
- Redo: the existing filter stack is undo-only. Is that acceptable for structural mutations, or does
  #31 require real redo?
- Scope boundary with `Graph::edgeFilterUnilateral()`/`relationSet()` style non-destructive
  operations that already have their own history mechanism — should structural undo reuse
  `m_visibilityHistory` or be a genuinely separate stack?

## Work Rules

Same as elsewhere in this codebase: `./scripts/run_golden_compares.sh` clean before any commit;
prefer adapters/extensions of the existing snapshot pattern over introducing new machinery
(`QUndoStack`) unless a concrete limitation of the snapshot approach is found; every phase
individually regression-tested before the next begins.
