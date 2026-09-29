# Signed Network Analysis / Structural Balance (WS18)

## Goal

First-class support for signed networks — graphs where edge sign (not just magnitude) is
meaningful: friend/enemy, trust/distrust, alliance/conflict. Covers the full chain from "don't
silently corrupt results on negative weights" through dedicated signed-graph algorithms:
Bellman-Ford/Johnson's-based distances, purpose-built signed centrality measures, and
Heider/Cartwright-Harary structural balance analysis on triads.

## Status

Tracked by #284. **P0-P3 complete** (v3.8-cycle). P4 (structural balance) scoped below, not
started.

## What WS18 Delivered

- **P0 — Parser support for negative edge weights** (#285) — DL and Adjacency (one-mode) parsers
  silently dropped negative-weight edges (`edgeWeight > 0` gate); fixed to `!= 0`, matching the
  other five formats. Golden coverage in all 7 supported formats, plus IO-roundtrip coverage.
- **P1 — Guard existing distance-based measures against negative weights** (#277) — Dijkstra is
  mathematically undefined for negative weights; every distance-derived measure now refuses
  cleanly via `Graph::negativeWeightsDetected()` instead of silently returning wrong numbers. All
  15 real call sites of `graphDistancesGeodesic()` guarded.
- **P2 — Johnson's algorithm** (negative-weight-safe shortest paths) — a global Bellman-Ford
  potential pass reweights edges non-negative, then the existing `dijkstraSSSP()` runs unmodified;
  negative-cycle detection falls out for free. Opt-in via `Graph::graphDistancesGeodesicSigned()`,
  not a silent auto-switch. GUI wiring on the four affected Graph-distances actions (Distance,
  Average Distance, Geodesic Distances Matrix, Diameter), each offering the upgrade on refusal.
  Two real bugs found and fixed along the way (a `dist_w > 0` vs `>= 0` Dijkstra edge case; a
  compute-mode cache not invalidating across plain/signed switches).
- **P3 — Signed-specific centrality measures**: signed degree centrality (#300 — pos/neg/ratio/net
  variants, out-degree only) and PN centrality (#301, Everett & Borgatti 2014 — binary ±1 model,
  three modes `all`/`out`/`in` depending on directedness). Both fully wired end-to-end: engine
  (`graph_centrality_signed_degree.cpp`, `graph_centrality_pn.cpp`), CLI kernel
  (`kernel_prominence_v4.cpp`), GUI (menu actions, dialogs, Prominence toolbox combo — deliberately
  excluded from "Visualize by prominence index" and Node Find, since neither measure has a single
  standardized score), reporting, and WS12 `--interactive-script` commands
  (`report-centrality-degree-signed`, `report-centrality-pn`). Both independently verified against
  an established outside reference implementation, not just self-consistency.
- Current signed-network architecture (Johnson's engine, `PNMode`, signed matrix construction)
  lives in [`README_DEVELOPER_NOTES.md`](../README_DEVELOPER_NOTES.md), not here.

**Unrelated fix found and landed along the way (#283):** cross-checking `dijkstraSSSP()` against
an independent library while building P2 surfaced a real BC bug in the existing (unsigned)
Dijkstra path, pre-dating this workstream. Fixed separately (`f6076bc7`).

## Known Gaps

- **What's This/tooltip text audit** — all 16 pre-existing centrality/prestige `QAction`s predate
  the richer Meaning/When to use/Weights/Compare-to doc-comment convention; signed degree and PN's
  own text follows it, but upgrading the other 15 is a separate, sizeable audit, not done here.
- **Signed geodesic distance semantics** — on a signed graph an individual shortest distance can
  itself be negative (confirmed on `Signed_Dir_N4_NoCycle.paj`, B→C = -2). Whether "Average
  Distance"/"Diameter" framing/wording still makes sense for a signed graph is undecided; applies
  to all four P2 GUI actions.
- **Follow-up issue #302** (adjacency-import dialog UX — delimiter dialog and row-mismatch error
  don't mention trailing delimiters) — found incidentally during P3 manual testing, filed and
  explicitly deferred, unrelated to signed networks specifically.

## What Remains Open — P4: Structural balance (Heider / Cartwright-Harary)

**Scoped for 3.9** (#305): the classical, undirected, well-defined core with no open design
questions.

- Classify each triad as **balanced** or **unbalanced** by the product-of-signs rule (positive
  product ⇔ balanced; equivalently 0 or 2 negative edges is balanced, 1 or 3 is not). Builds on
  `graphTriadCensus()`'s existing enumeration/MAN classification infrastructure
  (`src/graph/clustering/graph_triad_census.cpp`) by adding a sign dimension alongside the existing
  MAN dimension — needs a small design pass on how the two classifications compose (a triad is
  classified by both dyad-structure type *and* balance status), not a replacement of the existing
  function.
- Network-level **balance ratio** (fraction of balanced triads), following the same
  aggregate-from-per-triad-classification pattern the existing triad census already uses for its
  type-frequency table.
- Undirected graphs only for this pass — Cartwright-Harary balance theory is classically defined
  for undirected signed graphs, matching the classical scope.
- Compute-first, no UI beyond a report (matching how WS11's other measures shipped): no canvas
  coloring by balance status, no dedicated signed-network layout.

**Deferred past 3.8**, each needing its own scoping pass before implementation:

- **Directed-graph balance semantics** (#303) — the triad census already handles directed MAN
  types, so extending balance to directed graphs is plausible, but needs an explicit design
  decision (not an assumption) on what "balanced" means for a directed signed triad.
- **Clusterizable / two-faction test** (#304) — structural balance's strong theorem (a
  fully-balanced signed graph is exactly two mutually-hostile, internally-friendly factions).
  Stretch goal once the base classification lands.
- **Weighted/graded balance measures** beyond the classic ±1 sign model (e.g. degree-of-imbalance
  metrics beyond the simple balance ratio) — not ruled out permanently, just not filed yet.

Each phase gets its own issue once actually scoped, per this repo's usual discipline.
