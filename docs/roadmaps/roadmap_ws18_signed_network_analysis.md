# Signed Network Analysis / Structural Balance (WS18)

## Goal

First-class support for signed networks — graphs where edge sign (not just magnitude) is
meaningful: friend/enemy, trust/distrust, alliance/conflict. Covers the full chain from "don't
silently corrupt results on negative weights" through dedicated signed-graph algorithms:
Bellman-Ford-based distances, purpose-built signed centrality measures, and Heider/Cartwright-
Harary structural balance analysis on triads.

## Status

**P1 complete (2026-09-19, #277).** P2-P4 not started — scoped only.

**Unrelated fix found and landed along the way (#283):** while designing P2's Bellman-Ford engine
path, cross-checking `dijkstraSSSP()`'s behavior against an independent library surfaced a real BC
bug in the existing (unsigned) Dijkstra path — unrelated to signed networks, pre-dating this
workstream. Fixed separately; see `f6076bc7`. Noted here only because it was found during WS18
work, not because it's in scope for this workstream.

## Background

SocNetV already treats negative edge weights as legitimate, intentional input — Settings has a
dedicated negative-edge display color and negative edges render dashed on the canvas. But almost
nothing downstream actually understands *sign* as distinct from *magnitude*:

- **`DistanceEngine::dijkstraSSSP()`** (`src/engine/distance_engine.cpp:1196`) is textbook
  Dijkstra — mathematically undefined for negative weights (the "popped = finalized" invariant
  breaks). No guard anywhere. Every distance-derived measure (CC, BC, SC, EC, IRCC, PC) silently
  returns finite-but-wrong numbers on a negative-weight graph. This is #277.
- **One existing foothold:** `centralityBonacich()`
  (`src/graph/centrality/graph_centrality_bonacich.cpp`) already allows negative `β`
  (Bonacich & Lloyd 2004's extension of Bonacich Power Centrality to negative ties) — per-node
  scores can come out negative, unlike every other measure in the app. This is the only place in
  the codebase that treats sign as semantically meaningful rather than just "a weight that happens
  to be negative."
- **A partial, unsigned foothold:** `graphTriadCensus()`
  (`src/graph/clustering/graph_triad_census.cpp:42`) already computes the full 16-type
  Davis-Holland-Leinhardt MAN triad classification in parallel, with per-type doc comments. This is
  the *shape* half of what structural balance needs (triad enumeration + classification
  infrastructure already exists and is well-tested) — it does not currently look at edge sign at
  all, since MAN classification is about (mutual/asymmetric/null) dyad structure, not sign.
  Balance classification is an orthogonal layer on top, not a rewrite of this function.
- **No existing infrastructure for:** Bellman-Ford or any negative-weight-safe shortest-path
  algorithm; negative-cycle detection; any signed-specific centrality measure (PN centrality is not
  implemented); any structural-balance/triad-sign analysis.

## Relationship to #277

#277's guard (reject/warn on negative weights before Dijkstra runs) is **P1** of this workstream,
not a separate piece of work — it was scoped narrowly on its own issue because it was the
immediate correctness fix needed regardless of whether signed-network support was ever built out
further. Everything past P1 is new, larger, and was explicitly deferred pending prioritization —
see `roadmap_ws11_algorithm_additions.md`'s #277 entry.

## Phases

Ordered by dependency. P1 is complete (#277). P2 depends on P1 only in the sense that P1's guard
becomes obsolete for graphs where P2's engine is selected — the two aren't sequenced by data
dependency otherwise. P3 depends on P2 for any measure defined via signed shortest paths (most of
them are), but PN centrality specifically is not distance-based and could land before P2. P4
(structural balance) is independent of P2/P3 — it operates on triad sign patterns directly, not on
distances.

### P1 — Guard existing distance-based measures against negative weights (#277) ✔ complete

Shipped 2026-09-19 across three commits (c8666fd6, 5123064a, 38f1d6c1). Detects a negative edge
weight once per computation inside `DistanceEngine::initRun()` (merged into the existing
max-weight-scan loop — one O(N²) pass, not a second one — and checked before
`graph.setConnectedCached(true)` runs, so refusal never leaves connectivity state looking
legitimately computed), reported back via `Graph::negativeWeightsDetected()`, a queryable flag
mirroring the existing `m_progressCanceled`/`progressCanceled()` pattern exactly (same
cross-thread-atomic treatment, same reset-at-start-of-computation shape).

All 15 real `considerWeights`-forwarding call sites of `Graph::graphDistancesGeodesic()` are
guarded — 11 mirror an existing `progressCanceled()`-refusal precedent at their own call site
(`writeCentralityCloseness()`/`writeCentralityBetweenness()`/`writeCentralityStress()`/
`writeCentralityEccentricity()`/`writeCentralityPower()`/`writeEccentricity()` in
`graph_reports.cpp`; `centralityClosenessIR()`; `prestigeProximity()`;
`layoutByProminenceIndex()`; `vertexFindByIndexScore()`;
`graphMatrixShortestPathsCreate()`/`graphMatrixDistanceGeodesicCreate()` in
`graph_matrix_distances.cpp`). The remaining 4 (`graph_distance_facade.cpp`) had no existing
precedent and needed their own design: `graphDistanceGeodesic()`/`graphGeodesicDistanceDistribution()`
now return `RAND_MAX`/an empty `QMap` on refusal (matching `apspDistance()`'s own "nothing computed"
sentinel — without this, a *prior* successful computation's stale `m_apspDist` entry would have been
returned instead of a clear refusal signal); `graphDiameter()`/`graphDistanceGeodesicAverage()` have
no safe sentinel (0 is a legitimate result), so the fix lives in their sole MainWindow callers
(`slotAnalyzeDiameter()`/`slotAnalyzeDistanceAverage()` in `mainwindow_analyze_distance.cpp`), which
now capture `negativeWeightsDetected()` the same way they already captured `*isWeighted`/`*isConnected`
and show a refusal status message instead of a misleading numeric result.

Verified live via `--interactive-script` against a negative-weight fixture and an otherwise-identical
positive-weight control (two new script commands, `diameter`/`average-distance`, added since no
existing command reached those two MainWindow slots). `./scripts/run_golden_compares.sh` clean
throughout. Matrix-power measures (EVC, Katz, Bonacich, PRP) are not Dijkstra-based and were
correctly left untouched — Bonacich already handles negative values by design (see Background).

### P2 — Negative-weight-safe shortest paths (Bellman-Ford)

- New SSSP engine path for the negative-weight case: Bellman-Ford, O(V·E) per source (Johnson's
  algorithm — a Bellman-Ford re-weighting pass once, then Dijkstra per source — is the standard way
  to avoid paying O(V·E) per source on graphs with only a few negative edges; worth evaluating once
  this phase is actually scoped, rather than committing to naive per-source Bellman-Ford upfront).
- Negative-cycle detection is mandatory, not optional: a shortest path is undefined in a graph with
  a reachable negative cycle. Must surface as a distinct, clearly-worded refusal (different from
  P1's "negative weight, wrong algorithm" refusal) — a negative cycle isn't a Dijkstra-can't-do-this
  problem, it's a "shortest path doesn't exist" problem, and BC/CC/etc. need a defined answer (skip
  the pair? refuse the whole computation?) that doesn't exist in unsigned SNA and needs a decision
  before implementation.
- Once this lands, P1's guard on the Dijkstra path can either stay (as the default algorithm
  selection for non-negative graphs, since Dijkstra is faster) or the engine can auto-select
  Bellman-Ford whenever a negative weight is detected instead of refusing — a design choice to make
  when this phase is scoped, not assumed here.

### P3 — Signed-specific centrality measures

- **PN centrality (Everett & Borgatti 2014)** — the standard purpose-built positive/negative
  centrality measure for signed networks; not implementable by re-running an unsigned measure on
  `|weight|`, it's defined directly in terms of signed adjacency. New
  `src/graph/centrality/graph_centrality_pn.cpp`, following the existing per-measure file
  convention (see Katz/Bonacich as the most recent examples of this pattern, including the doc
  comment shape from `docs/README_DEVELOPER_NOTES.md`).
- Audit whether any other existing measure has a natural signed extension worth adding here (e.g.
  signed closeness via P2's engine) — deliberately not pre-committing to a list beyond PN centrality,
  since matching Everett & Borgatti's actual scope (rather than SocNetV inventing variants) is the
  right first target.

### P4 — Structural balance analysis (Heider / Cartwright-Harary)

- Classify each triad as **balanced** or **unbalanced** by the product-of-signs rule (a triad is
  balanced iff the product of its three edge signs is positive — equivalently, 0 or 2 negative
  edges is balanced, 1 or 3 is not). Builds on `graphTriadCensus()`'s existing enumeration/MAN
  classification infrastructure (`src/graph/clustering/graph_triad_census.cpp`) by adding a sign
  dimension alongside the existing MAN dimension — needs design attention on how the two
  classifications compose (a triad is classified by both dyad-structure type *and* balance
  status), not a replacement of the existing function.
- Network-level **balance ratio** (fraction of balanced triads) as a single summary statistic,
  following the same aggregate-from-per-triad-classification pattern the existing triad census
  already uses for its type-frequency table.
- Directed-graph handling needs explicit design: Cartwright-Harary balance theory is classically
  defined for undirected signed graphs; SocNetV's triad census already handles directed MAN types,
  so extending balance to directed graphs (rather than restricting this phase to undirected graphs
  only) needs its own scoping pass, not an assumption either way here.
- **Clusterizable / two-faction test** (structural balance's strong theorem: a fully-balanced
  signed graph is exactly two mutually-hostile internally-friendly factions) as a possible stretch
  goal once the base classification lands — lower priority, not part of the initial deliverable.

## Explicitly out of scope (for now)

- Any UI feature (canvas coloring by balance status, dedicated signed-network layout) — this
  workstream is compute-first, matching how WS11's other measures shipped (algorithm slice first,
  UI wiring as a small follow-on once the measure exists and has golden coverage).
- Weighted/graded balance measures beyond the classic ±1 sign model (e.g. degree-of-imbalance
  metrics beyond the simple balance ratio) — not ruled out permanently, just not in the initial
  scope.

## Work Rules

Same discipline as every other workstream:

- `./scripts/run_golden_compares.sh` clean before any commit; new golden baselines for each new
  computation, including at least one hand-verified signed fixture with an analytically-known
  answer per phase (mirroring how #281's Esfahanian-Hakimi work and the Katz/Bonacich additions
  were each verified against independently-computed ground truth, not just self-consistency).
- New algorithm slices stay QtCore-only per the `src/graph/` boundary rule; UI/reporting hooks (if
  and when P4's stretch UI work is prioritized) live in `src/graph/ui/` and `src/graph/reporting/`.
- Each phase gets its own issue once actually scoped — don't implement a whole phase as one PR;
  P2-P4 in particular each bundle several design decisions that need to be settled (and likely
  checked in with the user, per this repo's mid-task consult rule) before code, not during.
- Centrality/clustering doc comments follow the fixed **Meaning / When to use / Compare to / Math**
  shape from `docs/README_DEVELOPER_NOTES.md`.
