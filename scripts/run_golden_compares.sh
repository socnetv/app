#!/usr/bin/env bash
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

BASE="${ROOT_DIR}/src/tools/baselines"
BASE_REACH="${ROOT_DIR}/src/tools/baselines/reachability"
BASE_WALKS="${ROOT_DIR}/src/tools/baselines/walks"
BASE_PROM="${ROOT_DIR}/src/tools/baselines/prominence"
BASE_IO="${ROOT_DIR}/src/tools/baselines/io_roundtrip"
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
  "${BASE}/DunbarGelada_H22a__FT2__C1_W0_IW1_DI0.json"

run_case \
  "${DATA}/Stephenson_Zelen_Dunbar_Dunbar_Gelada_baboon_colony_H22a_IC.paj" \
  2 \
  -c 1 -w 1 -x 1 -k 0 \
  "${BASE}/DunbarGelada_H22a__FT2__C1_W1_IW1_DI0.json"

run_case \
  "${DATA}/Stokman_Ziegler_Corporate_Interlocks_Netherlands.dl" \
  5 \
  -c 1 -w 0 -x 1 -k 0 \
  "${BASE}/StokmanZiegler_Netherlands__FT5__C1_W0_IW1_DI0.json"

run_case \
  "${DATA}/Stokman_Ziegler_Corporate_Interlocks_Netherlands.dl" \
  5 \
  -c 1 -w 1 -x 1 -k 0 \
  "${BASE}/StokmanZiegler_Netherlands__FT5__C1_W1_IW1_DI0.json"

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

echo
if [[ "$FAILS" -eq 0 ]]; then
  echo "[OK] All golden comparisons passed."
  exit 0
else
  echo "[ERROR] Golden comparisons failed: $FAILS case(s)."
  exit 1
fi