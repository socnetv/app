# Signed Network Analysis / Structural Balance (WS18)

## Goal

First-class support for signed networks — graphs where edge sign (not just magnitude) is
meaningful: friend/enemy, trust/distrust, alliance/conflict. Covers the full chain from "don't
silently corrupt results on negative weights" through dedicated signed-graph algorithms:
Bellman-Ford/Johnson's-based distances, purpose-built signed centrality measures, and
Heider/Cartwright-Harary structural balance analysis on triads.

## Status

Tracked by #284. **P0-P4 implemented** (P0-P3 shipped in v3.8; P4 is on develop, pending the 3.9
release). #304 (clusterizable / two-faction test) in progress. #303 (directed balance semantics)
and the layout-exploration piece of #307 are scoped but not started - see What Remains Open.

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
- **P4 — Structural balance (Heider/Cartwright-Harary)** (#305): classifies each closed triad
  (all 3 dyads present) as **balanced** or **unbalanced** by the product-of-signs rule (positive
  product ⇔ balanced; equivalently 0 or 2 negative edges is balanced, 1 or 3 is not), plus a
  network-level **balance ratio** (fraction of balanced closed triads). Open triads (fewer than 3
  dyads present) are counted separately, excluded from the ratio. Undirected, signed graphs only
  - refuses cleanly (`Graph::graphStructuralBalance()`) on a directed graph or one with no
  negative-weight edge at all (the latter would otherwise trivially report 100%-balanced, a
  misleading result masking the absence of any signed structure to classify). A deliberately
  separate O(n³) pass (`graph_structural_balance.cpp`), not a dimension bolted onto
  `graphTriadCensus()`'s MAN classifier. Wired end-to-end: engine, CLI kernel
  (`kernel_signed_v10.cpp`), report writer (`writeStructuralBalance()`), GUI (menu action under
  Communities/Subgroups, Control Panel "Communities" combo entry - #307's non-layout scope).
  Independently verified against a from-scratch Python triad enumeration and cross-checked
  against R's `signnet` package's scoping convention (complete triangles only, matching
  `count_signed_triangles()`/`balance_score()`).
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

## What Remains Open

**In progress — Clusterizable / two-faction test** (#304): structural balance's strong theorem: a
fully-balanced signed graph is exactly two mutually-hostile, internally-friendly factions.
**Genuinely distinct from P4's triad ratio above, not a bigger version of it** — a network can
have zero closed triads at all (e.g. a 4-cycle A-B-C-D-A with signs +,+,+,-, only 4 of 6 pairs
tied) and still fail clusterizability, because full balance is a property of every cycle in the
graph, not just 3-vertex ones. Scoped algorithm: BFS/2-coloring walk (same shape as the classic
bipartite-check algorithm — positive edge means same faction as neighbor, negative edge means
opposite faction, a forced contradiction means not clusterizable), O(V+E) per component. This is
the correct/efficient algorithm for the plain yes/no + partition question, not a simplification —
nothing faster exists because the question itself is polynomial-time easy. GUI wiring (menu +
Control Panel entry) is in scope for #304 itself, unlike P4 which deferred it to #307.
Faction-based node coloring on the canvas is explicitly **out** of #304's scope and pushed to
#307 instead: it doesn't fit the existing "Layout by Prominence Index → Node Color" mechanism,
which maps a continuous score to a red/blue hue gradient (`graph_layouts_basic.cpp`), not a clean
fit for a binary 0/1 faction value — needs its own small, dedicated coloring path.

**Scoped, not started**, each needing its own scoping pass before implementation:

- **Directed-graph balance semantics** (#303) — the triad census already handles directed MAN
  types, so extending balance to directed graphs is plausible, but needs an explicit design
  decision (not an assumption) on what "balanced" means for a directed signed triad.
- **#307's layout-exploration remainder** — P4 and #304 both push balance-driven canvas layout
  out of their own scope; #307 itself only landed the menu/Control Panel wiring for P4, not any
  layout. Explored once already: no existing layout mechanism fits a per-triad ratio or a binary
  per-node faction value cleanly (see #304's note above) — revisit once #304's faction data
  exists, since that's the piece that actually makes a real layout (e.g. positioning by faction)
  feasible.
- **Frustration index** — if a network *isn't* fully balanced, the minimum number of ties that
  would need to be flipped/removed to make it balanced. Deliberately **not** part of #304: this is
  an NP-hard optimization problem, a different algorithm class entirely (needs an LP/ILP solver,
  not a graph walk) from the polynomial-time yes/no clusterizability check above. Parked as a
  future WS11 algorithm-addition candidate, referencing #304, not scoped or filed yet.
- **Weighted/graded balance measures** beyond the classic ±1 sign model (e.g. degree-of-imbalance
  metrics beyond the simple balance ratio) — not ruled out permanently, just not filed yet.

Each phase gets its own issue once actually scoped, per this repo's usual discipline.
