# Bipartite / Two-Mode Network Analysis (WS17)

## Goal

Add first-class support for bipartite (two-mode) networks: a persistent partition model on the
graph itself, round-trip file format support beyond the existing `.2sm`/UCINET DL import path,
two-mode-aware layouts and metrics, and a bipartite matching solver. Triggered by an external
LLM-drafted feature list a user's friend collected; most of that list assumed SocNetV had no
two-mode support at all, which is inaccurate — see Background.

## Status

**Not started.** Scoped only; no code written yet.

## Background

An external plan (LLM output, no visibility into this codebase) proposed a four-phase bipartite
feature set: data model changes, two-mode file formats, bipartite generators, sparse
biadjacency-matrix projections via Eigen/Armadillo, two-mode layouts and metrics, and finally a
matching solver plus a recommendation-engine feature.

Auditing the actual codebase against that plan found:

- **Already shipped, contrary to the plan's assumption:** `.2sm`/`.aff` two-mode sociomatrix import
  (`Parser::parseAsTwoModeSociomatrix()`, `src/parser/parser_adjacency.cpp:385-645`), with three
  import modes — raw bipartite graph, person/Mode-1 projection (`B·Bᵀ`), event/Mode-2 projection
  (`Bᵀ·B`) — selected via dialog
  (`src/mainwindow/network/mainwindow_network_file.cpp:757-790`). UCINET DL two-mode import via
  `NR=`/`NC=` headers also exists (`src/parser/parser_dl.cpp:417-700`), always as a bipartite
  directed graph, no projection option. Both are documented at
  `website/src/content/docs/manual/formats.mdx:698-894`. **Degree/closeness/betweenness centrality
  and PageRank already work correctly on bipartite graphs today** — they're relation-agnostic and
  need no bipartite-specific plumbing; the plan's proposal to add "bipartite degree centrality"
  etc. is a non-issue.
- **Already effectively covered:** `layoutByProminenceIndex()`
  (`src/graph/layouts/graph_layouts_basic.cpp:220`) already does concentric-circle-by-score
  layout for any prominence index — the plan's "concentric dual-ring layout" is a small variant of
  this (fixed inner/outer ring by set membership instead of by score), not new infrastructure.
- **Genuine gaps:** no persistent bipartite/partition flag on `Graph`/`GraphVertex` (two-mode-ness
  today is only encoded as node color/shape at parse time, discarded semantically after import); no
  GraphML or Pajek two-mode round-trip; no `.2sm` export; no bipartite network generators; no
  two-column/parallel layout; no Robins-Alexander (4-cycle-based) two-mode clustering coefficient;
  no bipartite matching solver (no Hopcroft-Karp/Hungarian, no augmenting-path infrastructure of
  any kind — the existing Edmonds-Karp max-flow in `src/graph/cohesion/graph_connectivity.cpp` is
  for vertex connectivity, not matching, though architecturally related).
- **Explicitly descoped from the external plan:**
  - **Sparse biadjacency matrix + Eigen/Armadillo.** `Matrix` (`src/matrix.h/.cpp`) is dense,
    hand-rolled, no linear-algebra dependency anywhere in the codebase
    (`src/graph/matrices/graph_matrix_adjacency.cpp` is O(N²) dense by design). Introducing a
    third-party linear-algebra library for one feature is a large dependency/build-system decision,
    and it isn't even needed — projection is already solved via direct edge construction at parse
    time, not matrix multiplication. **No external dependencies will be introduced for this
    workstream; every algorithm here is implemented in-house**, consistent with the rest of the
    codebase.
  - **Recommendation engine (collaborative filtering / random-walk-with-restart).** Out of scope —
    a "recommender system" feature doesn't fit SocNetV's identity as an SNA analysis/visualization
    tool. PageRank already exists; a personalized/RWR variant is not planned here.

## Phases

Ordered by dependency, not by the external plan's original phase numbering.

### P1 — Persistent bipartite/partition model

Foundational; P3 (dual-ring layout variant) and P5 (Robins-Alexander) depend on it.

- Add a partition/mode attribute to `GraphVertex` (which set/mode a node belongs to) and a
  bipartite flag to `Graph`.
- On bipartite-mode import (`.2sm`, UCINET DL two-mode), populate the partition attribute directly
  instead of only setting color/shape.
- Decide whether edge insertion should be validated/warned against when it would create an
  intra-set edge in strict bipartite mode (per the external plan's suggestion) — needs a UX call on
  whether this is a hard constraint or advisory, since SocNetV otherwise doesn't restrict edge
  creation by node attributes anywhere else.

### P2 — GraphML & Pajek two-mode round-trip, `.2sm` export

- GraphML: custom key/value pair(s) to persist partition membership; round-trip load/save.
- Pajek: parse/write the native two-mode `*Vertices` convention.
- `.2sm` export: currently import-only (per `formats.mdx:875`, "not yet supported") — add the
  write side now that P1 gives a real partition attribute to export from.

### P3 — Two-column / parallel layout, dual-ring layout variant

- New two-column layout: Set A in one vertical column, Set B in another, cross-set edges only.
- Dual-ring variant of the existing `layoutByProminenceIndex()` concentric layout: ring assignment
  by partition membership (inner/outer) instead of by prominence score. Depends on P1 for partition
  data.

### P4 — Bipartite network generators

Follows the existing per-generator dialog + `Graph` method pattern
(`src/graph/generators/graph_random_networks.cpp`, `src/forms/dialograndErdosRenyi.*` etc.):

- Complete bipartite `K_{m,n}` generator.
- Bipartite Erdős–Rényi, `G(n₁, n₂, p)`.
- Bipartite preferential-attachment/scale-free variant (stretch — lower priority than the first
  two).

### P5 — Robins-Alexander two-mode clustering coefficient

- New formula in `src/graph/clustering/graph_clustering_coefficients.cpp`, built on 4-cycles
  instead of triangles (triangles are impossible in a proper two-mode graph, so the existing
  Watts-Strogatz clustering coefficient is meaningless here). Reuses the existing
  neighborhood-enumeration/caching pattern (`hasCLC()`/`CLC()`) but needs P1's partition data to
  enumerate 4-cycles correctly.

### P6 — Bipartite matching (Hopcroft-Karp)

Largest standalone lift; no dependency on P1-P5, could be built independently.

- Maximum-cardinality bipartite matching (Hopcroft-Karp), hand-rolled, no external dependency.
- Minimum-weight matching / assignment problem (Hungarian algorithm) as a follow-on once
  maximum-cardinality matching lands, for weighted use cases (e.g. worker↔task assignment).
- UI: highlight matched edges on the canvas.

## Explicitly out of scope

- Sparse matrix representation / Eigen / Armadillo or any other third-party linear-algebra
  dependency — see Background.
- Recommendation-engine features (collaborative filtering, random-walk-with-restart
  personalized PageRank) — doesn't fit the app's scope as an SNA tool.

## Work Rules

Same discipline as every other workstream:

- `./scripts/run_golden_compares.sh` clean before any commit; new golden baselines for each new
  computation (generators, clustering coefficient, matching).
- New algorithm slices stay QtCore-only per the `src/graph/` boundary rule; UI/reporting hooks live
  in `src/graph/ui/` and `src/graph/reporting/`.
- Each phase gets its own issue(s) once actually scoped — don't implement a whole phase as one PR.
- Centrality/clustering doc comments follow the fixed **Meaning / When to use / Compare to / Math**
  shape from `docs/README_DEVELOPER_NOTES.md`.
