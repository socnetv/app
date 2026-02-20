#!/usr/bin/env bash
set -uo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CLI="${ROOT_DIR}/build/socnetv-cli"
if [[ ! -x "$CLI" ]]; then
  CLI="$ROOT_DIR/builds/__unspec__/Debug/socnetv-cli"
fi

BASE="${ROOT_DIR}/src/tools/baselines"
BASE_REACH="${ROOT_DIR}/src/tools/baselines/reachability"
BASE_WALKS="${ROOT_DIR}/src/tools/baselines/walks"
BASE_PROM="${ROOT_DIR}/src/tools/baselines/prominence"
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
  local baseline="${@: -1}"       # last arg

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
  local baseline="${@: -1}"

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
  local baseline="${@: -1}"

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
  local baseline="${@: -1}"

  echo "==> $(basename "$baseline")"
  if ! "$CLI" --kernel prominence -i "$input" -f "$ftype" "${flags[@]}" --compare-json "$baseline"; then
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

echo
if [[ "$FAILS" -eq 0 ]]; then
  echo "[OK] All golden comparisons passed."
  exit 0
else
  echo "[ERROR] Golden comparisons failed: $FAILS case(s)."
  exit 1
fi