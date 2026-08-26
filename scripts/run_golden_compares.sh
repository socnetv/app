#!/usr/bin/env bash
# run_golden_compares.sh — SocNetV CLI golden regression harness
#
# Runs all registered kernel cases and compares their output against
# committed JSON baseline files. Exits non-zero if any case fails.
#
# Usage:
#   ./scripts/run_golden_compares.sh
#   SOCNETV_CLI=./build/socnetv-cli ./scripts/run_golden_compares.sh
#
# Kernels covered:
#   v1  distance      — DistanceEngine + geodesic centralities
#   v2  reachability  — reachability matrix (R(i,j) = 1 if finite geodesic)
#   v3  walks_matrix  — walks matrix A^K
#   v4  prominence    — all node-level centrality + prestige indices
#   v5  io_roundtrip  — load → export → reload signature comparison
#                       (export skipped for formats without exporter;
#                        baseline locks in the skipped outcome too)
#   v6  clustering    — clustering coefficient + triad census + clique census
#   v7  connectivity  — weakly/strongly connected components count + per-node component IDs
#   v8  matrix        — raw contents of every Matrix-producing operation (adjacency,
#                        inverse, distances, similarity, reachability, walks, cliques)
#   v9  vertex_connectivity — local (Menger's theorem/max-flow) or global (pairwise-minimum)
#                        vertex connectivity
#
# Baselines:
#   src/tools/baselines/distance/     (v1)
#   src/tools/baselines/reachability/ (v2)
#   src/tools/baselines/walks/        (v3)
#   src/tools/baselines/prominence/   (v4)
#   src/tools/baselines/io_roundtrip/ (v5)
#   src/tools/baselines/clustering/   (v6)
#   src/tools/baselines/connectivity/ (v7)
#   src/tools/baselines/matrix/       (v8)
#   src/tools/baselines/vertex_connectivity/ (v9)
#
# To add a new case:
#   1. Run socnetv-cli --kernel <k> ... --dump-json <baseline.json>
#   2. Commit the baseline JSON
#   3. Add a run_case_<k> call below in the appropriate section
#
# To regenerate a baseline after a deliberate semantic fix:
#   Run the dump command again and commit the updated JSON.
#   Never regenerate baselines to silence a real regression.
set -uo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_TYPE="${BUILD_TYPE:-Debug}"  # Debug|Release (hint only)

# shellcheck source=/dev/null
. "$ROOT_DIR/scripts/lib/find_socnetv_cli.sh"

if [[ -n "${SOCNETV_CLI:-}" ]]; then
  CLI="$SOCNETV_CLI"
else
  CLI="$(find_socnetv_cli "$ROOT_DIR" "$BUILD_TYPE" 2>/dev/null || true)"
fi

if [[ -z "${CLI:-}" || ! -x "$CLI" ]]; then
  echo "[ERROR] socnetv-cli not found/executable." >&2
  echo "Hint: SOCNETV_CLI=/full/path/to/socnetv-cli $0" >&2
  exit 2
fi

echo "[golden] Using CLI: $CLI"

BASE_DISTANCE="${ROOT_DIR}/src/tools/baselines/distance"
BASE_REACH="${ROOT_DIR}/src/tools/baselines/reachability"
BASE_WALKS="${ROOT_DIR}/src/tools/baselines/walks"
BASE_PROM="${ROOT_DIR}/src/tools/baselines/prominence"
BASE_IO="${ROOT_DIR}/src/tools/baselines/io_roundtrip"
BASE_CLUST="${ROOT_DIR}/src/tools/baselines/clustering"
BASE_CONN="${ROOT_DIR}/src/tools/baselines/connectivity"
BASE_MATRIX="${ROOT_DIR}/src/tools/baselines/matrix"
BASE_VCONN="${ROOT_DIR}/src/tools/baselines/vertex_connectivity"
DATA="${ROOT_DIR}/src/data"

if [[ ! -x "$CLI" ]]; then
  echo "[ERROR] socnetv-cli not found/executable at: $CLI"
  echo "Build it first (e.g. cmake --build build -j)."
  exit 2
fi

FAILS=0

run_case() {
  local input="$1"
  local ftype="$2"
  local flags=("${@:3:${#}-3}")   # all but last arg
  local baseline="${!#}"       # last arg

  echo "==> $(basename "$baseline")"
  if ! "$CLI" -i "$input" -f "$ftype" "${flags[@]}" --compare-json "$baseline"; then
    echo "[FAIL] $(basename "$baseline")"
    FAILS=$((FAILS+1))
  fi
}

run_case_reachability() {
  local input="$1"
  local ftype="$2"
  local flags=("${@:3:${#}-3}")
  local baseline="${!#}"

  echo "==> $(basename "$baseline")"
  if ! "$CLI" --kernel reachability -i "$input" -f "$ftype" "${flags[@]}" --compare-json "$baseline"; then
    echo "[FAIL] $(basename "$baseline")"
    FAILS=$((FAILS+1))
  fi
}

run_case_walks() {
  local input="$1"
  local ftype="$2"
  local walks_len="$3"
  local flags=("${@:4:${#}-4}")
  local baseline="${!#}"

  echo "==> $(basename "$baseline")"
  if ! "$CLI" --kernel walks_matrix --walks-length "$walks_len" -i "$input" -f "$ftype" "${flags[@]}" --compare-json "$baseline"; then
    echo "[FAIL] $(basename "$baseline")"
    FAILS=$((FAILS+1))
  fi
}

run_case_prominence() {
  local input="$1"
  local ftype="$2"
  local flags=("${@:3:${#}-3}")
  local baseline="${!#}"

  echo "==> $(basename "$baseline")"
  if ! "$CLI" --kernel prominence -i "$input" -f "$ftype" "${flags[@]}" --compare-json "$baseline"; then
    echo "[FAIL] $(basename "$baseline")"
    FAILS=$((FAILS+1))
  fi
}

run_case_clustering() {
  local input="$1"
  local ftype="$2"
  local flags=("${@:3:${#}-3}")
  local baseline="${!#}"

  echo "==> $(basename "$baseline")"
  if ! "$CLI" --kernel clustering -i "$input" -f "$ftype" "${flags[@]}" --compare-json "$baseline"; then
    echo "[FAIL] $(basename "$baseline")"
    FAILS=$((FAILS+1))
  fi
}

run_case_connectivity() {
  local input="$1"
  local ftype="$2"
  local flags=("${@:3:${#}-3}")
  local baseline="${!#}"

  echo "==> $(basename "$baseline")"
  if ! "$CLI" --kernel connectivity -i "$input" -f "$ftype" "${flags[@]}" --compare-json "$baseline"; then
    echo "[FAIL] $(basename "$baseline")"
    FAILS=$((FAILS+1))
  fi
}

run_case_matrix() {
  local input="$1"
  local ftype="$2"
  local baseline="${!#}"

  echo "==> $(basename "$baseline")"
  if ! "$CLI" --kernel matrix -i "$input" -f "$ftype" -c 0 --compare-json "$baseline"; then
    echo "[FAIL] $(basename "$baseline")"
    FAILS=$((FAILS+1))
  fi
}

run_case_vertex_connectivity() {
  local input="$1"
  local ftype="$2"
  local flags=("${@:3:${#}-3}")
  local baseline="${!#}"

  echo "==> $(basename "$baseline")"
  if ! "$CLI" --kernel vertex_connectivity -i "$input" -f "$ftype" "${flags[@]}" --compare-json "$baseline"; then
    echo "[FAIL] $(basename "$baseline")"
    FAILS=$((FAILS+1))
  fi
}

run_case_io() {
  local input="$1"
  local ftype="$2"
  shift 2

  local baseline="${!#}"
  local flags=()
  if (( $# > 1 )); then
    flags=("${@:1:$#-1}")
  fi

  echo "==> $(basename "$baseline")"
  if ! "$CLI" --kernel io_roundtrip -i "$input" -f "$ftype" \
       ${flags[@]+"${flags[@]}"} --compare-json "$baseline"; then
    echo "[FAIL] $(basename "$baseline")"
    FAILS=$((FAILS+1))
  fi
}

# --- Cases (extend this list as kernels grow) ---

# DISTANCE (schema v1)
run_case \
  "${DATA}/Stephenson_Zelen_Dunbar_Dunbar_Gelada_baboon_colony_H22a_IC.paj" \
  2 \
  -c 1 -w 0 -x 1 -k 0 \
  "${BASE_DISTANCE}/DunbarGelada_H22a__FT2__C1_W0_IW1_DI0.json"

run_case \
  "${DATA}/Stephenson_Zelen_Dunbar_Dunbar_Gelada_baboon_colony_H22a_IC.paj" \
  2 \
  -c 1 -w 1 -x 1 -k 0 \
  "${BASE_DISTANCE}/DunbarGelada_H22a__FT2__C1_W1_IW1_DI0.json"

run_case \
  "${DATA}/Stokman_Ziegler_Corporate_Interlocks_Netherlands.dl" \
  5 \
  -c 1 -w 0 -x 1 -k 0 \
  "${BASE_DISTANCE}/StokmanZiegler_Netherlands__FT5__C1_W0_IW1_DI0.json"

run_case \
  "${DATA}/Stokman_Ziegler_Corporate_Interlocks_Netherlands.dl" \
  5 \
  -c 1 -w 1 -x 1 -k 0 \
  "${BASE_DISTANCE}/StokmanZiegler_Netherlands__FT5__C1_W1_IW1_DI0.json"

# REACHABILITY (schema v2)
run_case_reachability \
  "${DATA}/Stephenson_Zelen_Dunbar_Dunbar_Gelada_baboon_colony_H22a_IC.paj" \
  2 \
  -w 1 -x 1 -k 0 -c 0 \
  "${BASE_REACH}/DunbarGelada_H22a__REACH__V2.json"

run_case_reachability \
  "${DATA}/Stokman_Ziegler_Corporate_Interlocks_Netherlands.dl" \
  5 \
  -w 1 -x 1 -k 0 -c 0 \
  "${BASE_REACH}/StokmanZiegler_Netherlands__REACH__V2.json"

# WALKS MATRIX (schema v3)
run_case_walks \
  "${DATA}/Stephenson_Zelen_Dunbar_Dunbar_Gelada_baboon_colony_H22a_IC.paj" \
  2 \
  6 \
  -w 1 -x 1 -k 0 -c 0 \
  "${BASE_WALKS}/DunbarGelada_H22a__WALKS_K6__V3.json"

run_case_walks \
  "${DATA}/Stokman_Ziegler_Corporate_Interlocks_Netherlands.dl" \
  5 \
  6 \
  -w 1 -x 1 -k 0 -c 0 \
  "${BASE_WALKS}/StokmanZiegler_Netherlands__WALKS_K6__V3.json"

run_case_walks \
  "${DATA}/TinyPath_N3_E2.paj" \
  2 \
  2 \
  -w 1 -x 1 -k 0 -c 0 \
  "${BASE_WALKS}/TinyPath_N3_E2__WALKS_K2__V3.json"

# PROMINENCE (schema v4)
run_case_prominence \
  "${DATA}/TinyPath_N3_E2.paj" \
  2 \
  -w 0 -x 1 -k 0 \
  "${BASE_PROM}/TinyPath_N3_E2__PROM__V4__FT2__W0_IW1_DI0.json"

run_case_prominence \
  "${DATA}/TinyDirChain_N3.paj" \
  2 \
  -w 0 -x 1 -k 0 \
  "${BASE_PROM}/TinyDirChain_N3__PROM__V4__FT2__W0_IW1_DI0.json"

run_case_prominence \
  "${DATA}/Krackhardt_Kite_N10.paj" \
  2 \
  -w 0 -x 1 -k 0 \
  "${BASE_PROM}/Krackhardt_Kite_N10__PROM__V4__FT2__W0_IW1_DI0.json"

run_case_prominence \
  "${DATA}/Krackhardt_Kite_N10.paj" \
  2 \
  -w 1 -x 1 -k 0 \
  "${BASE_PROM}/Krackhardt_Kite_N10__PROM__V4__FT2__W1_IW1_DI0.json"

run_case_prominence \
  "${DATA}/Sampson_Monks_N18.net" \
  2 \
  -w 0 -x 1 -k 0 \
  "${BASE_PROM}/Sampson_Monks_N18__PROM__V4__FT2__W0_IW1_DI0.json"

# Weighted + isolate coverage (previously missing - see kernel_prominence_v4.cpp's
# centralityInformation()/centralityEigenvector() arg-shift fix, which this combination
# would have caught: considerWeights=1 with an isolate present, varying inverseWeights and
# dropIsolates independently).
run_case_prominence \
  "${DATA}/TinyWeightedIsolate_Undir_N4_E2.paj" \
  2 \
  -w 1 -x 1 -k 0 \
  "${BASE_PROM}/TinyWeightedIsolate_Undir_N4_E2__PROM__V4__FT2__W1_IW1_DI0.json"

run_case_prominence \
  "${DATA}/TinyWeightedIsolate_Undir_N4_E2.paj" \
  2 \
  -w 1 -x 1 -k 1 \
  "${BASE_PROM}/TinyWeightedIsolate_Undir_N4_E2__PROM__V4__FT2__W1_IW1_DI1.json"

run_case_prominence \
  "${DATA}/TinyWeightedIsolate_Undir_N4_E2.paj" \
  2 \
  -w 1 -x 0 -k 0 \
  "${BASE_PROM}/TinyWeightedIsolate_Undir_N4_E2__PROM__V4__FT2__W1_IW0_DI0.json"

# Katz/Bonacich on binary vs. real (non-unit) weights, both raw and inverted (WS11, #10/#39) -
# closes a verification gap: every other Katz/Bonacich baseline above only ever exercised binary
# adjacency (considerWeights=0), never real edge weights or the inverseWeights transformation.
# All three computed independently via Gauss-Jordan elimination in plain Python against the same
# A-B(2.5)-C(1.5) weighted path (isolate D dropped) before being dumped.
run_case_prominence \
  "${DATA}/TinyWeightedIsolate_Undir_N4_E2.paj" \
  2 \
  -w 0 -x 1 -k 1 --katz-alpha 0.2 --bonacich-alpha 1 --bonacich-beta 0.3 \
  "${BASE_PROM}/TinyWeightedIsolate_Undir_N4_E2__PROM__V4__FT2__W0_IW1_DI1_KA0.2_BA1_BB0.3.json"

run_case_prominence \
  "${DATA}/TinyWeightedIsolate_Undir_N4_E2.paj" \
  2 \
  -w 1 -x 0 -k 1 --katz-alpha 0.171499 --bonacich-alpha 1 --bonacich-beta 0.171499 \
  "${BASE_PROM}/TinyWeightedIsolate_Undir_N4_E2__PROM__V4__FT2__W1_IW0_DI1_KA0.171499_BA1_BB0.171499.json"

run_case_prominence \
  "${DATA}/TinyWeightedIsolate_Undir_N4_E2.paj" \
  2 \
  -w 1 -x 1 -k 1 --katz-alpha 0.2 --bonacich-alpha 1 --bonacich-beta 0.3 \
  "${BASE_PROM}/TinyWeightedIsolate_Undir_N4_E2__PROM__V4__FT2__W1_IW1_DI1_KA0.2_BA1_BB0.3.json"

# Katz Centrality (WS11, #10) - each value independently cross-checked against a hand-derived
# reference computation (Gauss-Jordan elimination of (I - alpha*A^T) in plain Python) before
# being dumped, not just accepted as "whatever the code produced."
run_case_prominence \
  "${DATA}/TinyPath_N3_E2.paj" \
  2 \
  -w 0 -x 1 -k 0 --katz-alpha 0.2 \
  "${BASE_PROM}/TinyPath_N3_E2__PROM__V4__FT2__W0_IW1_DI0_KA0.2.json"

run_case_prominence \
  "${DATA}/TinyDirChain_N3.paj" \
  2 \
  -w 0 -x 1 -k 0 --katz-alpha 0.5 \
  "${BASE_PROM}/TinyDirChain_N3__PROM__V4__FT2__W0_IW1_DI0_KA0.5.json"

# alpha=0.8 exceeds this graph's 1/lambda_max (~0.707) - locks in the boundary-rejection path
# (all-zero KC/SKC), not just the happy path.
run_case_prominence \
  "${DATA}/TinyPath_N3_E2.paj" \
  2 \
  -w 0 -x 1 -k 0 --katz-alpha 0.8 \
  "${BASE_PROM}/TinyPath_N3_E2__PROM__V4__FT2__W0_IW1_DI0_KA0.8_reject.json"

run_case_prominence \
  "${DATA}/Krackhardt_Kite_N10.paj" \
  2 \
  -w 0 -x 1 -k 0 --katz-alpha 0.1 \
  "${BASE_PROM}/Krackhardt_Kite_N10__PROM__V4__FT2__W0_IW1_DI0_KA0.1.json"

# Bonacich Power Centrality (WS11, #39) - each value independently cross-checked against a
# hand-derived reference computation (Gauss-Jordan elimination of (I - beta*A^T) in plain Python)
# before being dumped, same discipline as Katz above.
run_case_prominence \
  "${DATA}/TinyPath_N3_E2.paj" \
  2 \
  -w 0 -x 1 -k 0 --bonacich-alpha 1 --bonacich-beta 0.5 \
  "${BASE_PROM}/TinyPath_N3_E2__PROM__V4__FT2__W0_IW1_DI0_BA1_BB0.5.json"

run_case_prominence \
  "${DATA}/TinyDirChain_N3.paj" \
  2 \
  -w 0 -x 1 -k 0 --bonacich-alpha 1 --bonacich-beta -0.5 \
  "${BASE_PROM}/TinyDirChain_N3__PROM__V4__FT2__W0_IW1_DI0_BA1_BB-0.5.json"

# beta=0.75 exceeds this graph's 1/lambda_max (~0.707) - locks in the boundary-rejection path
# (all-zero BPC/SBPC), not just the happy path.
run_case_prominence \
  "${DATA}/TinyPath_N3_E2.paj" \
  2 \
  -w 0 -x 1 -k 0 --bonacich-alpha 1 --bonacich-beta 0.75 \
  "${BASE_PROM}/TinyPath_N3_E2__PROM__V4__FT2__W0_IW1_DI0_BA1_BB0.75_reject.json"

# beta=-0.6 (negative, within bound) deliberately exercises Bonacich's signature sign-flip
# behavior: node 2 (tied to both endpoints) gains a large positive score while both endpoints
# come out NEGATIVE - hand-verified via the same Gauss-Jordan cross-check.
run_case_prominence \
  "${DATA}/TinyPath_N3_E2.paj" \
  2 \
  -w 0 -x 1 -k 0 --bonacich-alpha 1 --bonacich-beta -0.6 \
  "${BASE_PROM}/TinyPath_N3_E2__PROM__V4__FT2__W0_IW1_DI0_BA1_BB-0.6_negflip.json"

run_case_prominence \
  "${DATA}/Krackhardt_Kite_N10.paj" \
  2 \
  -w 0 -x 1 -k 0 --bonacich-alpha 1 --bonacich-beta 0.1 \
  "${BASE_PROM}/Krackhardt_Kite_N10__PROM__V4__FT2__W0_IW1_DI0_BA1_BB0.1.json"

# IO ROUNDTRIP (schema v5)
run_case_io "${DATA}/TinyAdj_Undir_N3.adj" 3 -d " " -l 0 "${BASE_IO}/TinyAdj_Undir_N3__FT3.json"
run_case_io "${DATA}/TinyAdj_Weighted_Dir_N3.adj" 3 -d " " -l 0 "${BASE_IO}/TinyAdj_Weighted_Dir_N3__FT3.json"
run_case_io "${DATA}/TinyGraphML_Weighted_Dir_N3.graphml" 1 "${BASE_IO}/TinyGraphML_Weighted_Dir_N3__FT1.json"
run_case_io "${DATA}/Padgett_Florentine_Families.paj" 2 "${BASE_IO}/Padgett_Florentine_Families__FT2_multirel.json"
run_case_io "${DATA}/Benchmark_BA_Directed_N500_m3.paj" 2 "${BASE_IO}/Benchmark_BA_Directed_N500_m3__FT2_big.json"

# Skipped-export formats still get compared (they should remain skipped)
run_case_io "${DATA}/TinyGraphviz_Dir_N3.dot" 4 "${BASE_IO}/TinyGraphviz_Dir_N3__FT4.json"
run_case_io "${DATA}/TinyGML_Weighted_Dir_N3.gml" 6 "${BASE_IO}/TinyGML_Weighted_Dir_N3__FT6_weighted.json"
run_case_io "${DATA}/TinyEdgeList_Weighted_Dir_N3.wlst" 7 "${BASE_IO}/TinyEdgeList_Weighted_Dir_N3__FT7.json"
run_case_io "${DATA}/TinyGraphviz_Undir_N3.dot" 4 "${BASE_IO}/TinyGraphviz_Undir_N3__FT4.json"

# UCINET FT5 (export not supported — load + signature baseline only)
run_case_io "${DATA}/Stokman_Ziegler_Corporate_Interlocks_Netherlands.dl" 5 "${BASE_IO}/StokmanZiegler_Netherlands__FT5__IO__V5.json"
run_case_io "${DATA}/Bernard_Killworth_Fraternity.dl" 5 "${BASE_IO}/Bernard_Killworth_Fraternity__FT5__IO__V5.json"


# CLUSTERING (schema v6)
run_case_clustering \
  "${DATA}/Krackhardt_Kite_N10.paj" \
  2 \
  -w 0 -x 1 -k 0 \
  "${BASE_CLUST}/Krackhardt_Kite_N10__CLUST__V6__FT2__W0_IW1_DI0.json"

run_case_clustering \
  "${DATA}/TinyDirChain_N3.paj" \
  2 \
  -w 0 -x 1 -k 0 \
  "${BASE_CLUST}/TinyDirChain_N3__CLUST__V6__FT2__W0_IW1_DI0.json"

run_case_clustering \
  "${DATA}/TinyPath_N3_E2.paj" \
  2 \
  -w 0 -x 1 -k 0 \
  "${BASE_CLUST}/TinyPath_N3_E2__CLUST__V6__FT2__W0_IW1_DI0.json"

run_case_clustering \
  "${DATA}/Sampson_Monks_N18.net" \
  2 \
  -w 0 -x 1 -k 0 \
  "${BASE_CLUST}/Sampson_Monks_N18__CLUST__V6__FT2__W0_IW1_DI0.json"

run_case_clustering \
  "${DATA}/Stokman_Ziegler_Corporate_Interlocks_Netherlands.dl" \
  5 \
  -w 0 -x 1 -k 0 \
  "${BASE_CLUST}/StokmanZiegler_Netherlands__CLUST__V6__FT5__W0_IW1_DI0.json"

run_case_clustering \
  "${DATA}/Stokman_Ziegler_Corporate_Interlocks_Netherlands.dl" \
  5 \
  -w 1 -x 1 -k 0 \
  "${BASE_CLUST}/StokmanZiegler_Netherlands__CLUST__V6__FT5__W1_IW1_DI0.json"

run_case_clustering \
  "${DATA}/Stephenson_Zelen_Dunbar_Dunbar_Gelada_baboon_colony_H22a_IC.paj" \
  2 \
  -w 0 -x 1 -k 0 \
  "${BASE_CLUST}/DunbarGelada_H22a__CLUST__V6__FT2__W0_IW1_DI0.json"

run_case_clustering \
  "${DATA}/Stephenson_Zelen_Dunbar_Dunbar_Gelada_baboon_colony_H22a_IC.paj" \
  2 \
  -w 1 -x 1 -k 0 \
  "${BASE_CLUST}/DunbarGelada_H22a__CLUST__V6__FT2__W1_IW1_DI0.json"

# CONNECTIVITY (schema v7)
run_case_connectivity \
  "${DATA}/TinyDisconnected_Undir_N6_E4.paj" \
  2 \
  "${BASE_CONN}/TinyDisconnected_Undir_N6_E4__CONN__V7__FT2.json"

run_case_connectivity \
  "${DATA}/TinyDisconnected_Dir_N5_E3.paj" \
  2 \
  "${BASE_CONN}/TinyDisconnected_Dir_N5_E3__CONN__V7__FT2.json"

run_case_connectivity \
  "${DATA}/TinyPath_N3_E2.paj" \
  2 \
  "${BASE_CONN}/TinyPath_N3_E2__CONN__V7__FT2.json"

run_case_connectivity \
  "${DATA}/TinyIsolated_Dir_N2_E0.paj" \
  2 \
  "${BASE_CONN}/TinyIsolated_Dir_N2_E0__CONN__V7__FT2.json"

run_case_connectivity \
  "${DATA}/TinyArc_Dir_N2_E1.paj" \
  2 \
  "${BASE_CONN}/TinyArc_Dir_N2_E1__CONN__V7__FT2.json"

run_case_connectivity \
  "${DATA}/TinyWeaklyConn_Dir_N3_E2.paj" \
  2 \
  "${BASE_CONN}/TinyWeaklyConn_Dir_N3_E2__CONN__V7__FT2.json"

run_case_connectivity \
  "${DATA}/TinyIsolated_Undir_N3_E0.paj" \
  2 \
  "${BASE_CONN}/TinyIsolated_Undir_N3_E0__CONN__V7__FT2.json"

# Strong connectivity (--connectivity-type strong) - only meaningfully distinct from weak on
# directed graphs, so only the datasets above that actually parse as directed get a strong-mode
# baseline too (TinyIsolated_Dir_N2_E0 has zero edges and parses as undirected, so it's skipped).
run_case_connectivity \
  "${DATA}/TinyDisconnected_Dir_N5_E3.paj" \
  2 \
  --connectivity-type strong \
  "${BASE_CONN}/TinyDisconnected_Dir_N5_E3__CONN__V7__FT2__STRONG.json"

run_case_connectivity \
  "${DATA}/TinyArc_Dir_N2_E1.paj" \
  2 \
  --connectivity-type strong \
  "${BASE_CONN}/TinyArc_Dir_N2_E1__CONN__V7__FT2__STRONG.json"

run_case_connectivity \
  "${DATA}/TinyWeaklyConn_Dir_N3_E2.paj" \
  2 \
  --connectivity-type strong \
  "${BASE_CONN}/TinyWeaklyConn_Dir_N3_E2__CONN__V7__FT2__STRONG.json"

# MATRIX (schema v8) - see WS6.7 in roadmap_ws6_testing_ci_regression.md.
# Note: Benchmark_BA_Directed_N500_m3 is dumped in summary mode (row/col sums, trace,
# sampled cells) and skips the total_walks category - see kTotalWalksSkipThreshold in
# kernel_matrix_v8.cpp for why (total walks alone measured ~9 min at N=500).
run_case_matrix \
  "${DATA}/TinyPath_N3_E2.paj" \
  2 \
  "${BASE_MATRIX}/TinyPath_N3_E2__MATRIX__V8__FT2__W0_IW1_DI0.json"

run_case_matrix \
  "${DATA}/TinyDisconnected_Undir_N6_E4.paj" \
  2 \
  "${BASE_MATRIX}/TinyDisconnected_Undir_N6_E4__MATRIX__V8__FT2__W0_IW1_DI0.json"

run_case_matrix \
  "${DATA}/Benchmark_BA_Directed_N500_m3.paj" \
  2 \
  "${BASE_MATRIX}/Benchmark_BA_Directed_N500_m3__MATRIX__V8__FT2__W0_IW1_DI0.json"

# VERTEX CONNECTIVITY (schema v9) - deliberately Tiny*/toy datasets only. The global mode's
# pairwise-minimum algorithm is O(n^2) local-connectivity computations in the worst case (see
# Graph::graphConnectivity()'s doc comment) - fine for a handful of nodes, not for the
# Benchmark_*/500-node datasets used elsewhere in this file.
run_case_vertex_connectivity \
  "${DATA}/TinyPath_N3_E2.paj" \
  2 \
  --conn-mode global \
  "${BASE_VCONN}/TinyPath_N3_E2__VCONN__V9__FT2__global.json"

run_case_vertex_connectivity \
  "${DATA}/TinyPath_N3_E2.paj" \
  2 \
  --conn-mode local --conn-source 1 --conn-target 3 \
  "${BASE_VCONN}/TinyPath_N3_E2__VCONN__V9__FT2__local_1_3.json"

run_case_vertex_connectivity \
  "${DATA}/TinyPath_N3_E2.paj" \
  2 \
  --conn-mode local --conn-source 1 --conn-target 2 \
  "${BASE_VCONN}/TinyPath_N3_E2__VCONN__V9__FT2__local_1_2_adjacent.json"

run_case_vertex_connectivity \
  "${DATA}/TinyDisconnected_Undir_N6_E4.paj" \
  2 \
  --conn-mode global \
  "${BASE_VCONN}/TinyDisconnected_Undir_N6_E4__VCONN__V9__FT2__global.json"

run_case_vertex_connectivity \
  "${DATA}/TinyWeaklyConn_Dir_N3_E2.paj" \
  2 \
  --conn-mode global --connectivity-type weak \
  "${BASE_VCONN}/TinyWeaklyConn_Dir_N3_E2__VCONN__V9__FT2__global_weak.json"

run_case_vertex_connectivity \
  "${DATA}/TinyWeaklyConn_Dir_N3_E2.paj" \
  2 \
  --conn-mode global --connectivity-type strong \
  "${BASE_VCONN}/TinyWeaklyConn_Dir_N3_E2__VCONN__V9__FT2__global_strong.json"

run_case_vertex_connectivity \
  "${DATA}/TinyComplete_Undir_N4_E6.paj" \
  2 \
  --conn-mode global \
  "${BASE_VCONN}/TinyComplete_Undir_N4_E6__VCONN__V9__FT2__global.json"

echo
if [[ "$FAILS" -eq 0 ]]; then
  echo "[OK] All golden comparisons passed."
  exit 0
else
  echo "[ERROR] Golden comparisons failed: $FAILS case(s)."
  exit 1
fi