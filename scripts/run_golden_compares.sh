#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CLI="${ROOT_DIR}/build/socnetv-cli"
if [[ ! -x "$CLI" ]]; then
  CLI="$ROOT_DIR/builds/__unspec__/Debug/socnetv-cli"
fi
BASE="${ROOT_DIR}/src/tools/baselines"
BASE_REACH="${ROOT_DIR}/src/tools/baselines/reachability"
BASE_WALKS="${ROOT_DIR}/src/tools/baselines/walks"
DATA="${ROOT_DIR}/src/data"

if [[ ! -x "$CLI" ]]; then
  echo "[ERROR] socnetv-cli not found/executable at: $CLI"
  echo "Build it first (e.g. cmake --build build -j)."
  exit 2
fi

run_case() {
  local input="$1"
  local ftype="$2"
  local flags=("${@:3:${#}-3}")   # all but last arg
  local baseline="${@: -1}"       # last arg

  echo "==> $(basename "$baseline")"
  "$CLI" -i "$input" -f "$ftype" "${flags[@]}" --compare-json "$baseline"
}
run_case_reachability() {
  local input="$1"
  local ftype="$2"
  local flags=("${@:3:${#}-3}")   # all but last arg
  local baseline="${@: -1}"       # last arg

  echo "==> $(basename "$baseline")"
  "$CLI" --kernel reachability -i "$input" -f "$ftype" "${flags[@]}" --compare-json "$baseline"
}

run_case_walks() {
  local input="$1"
  local ftype="$2"
  local walks_len="$3"
  local flags=("${@:4:${#}-4}")   # all but last arg
  local baseline="${@: -1}"       # last arg

  echo "==> $(basename "$baseline")"
  "$CLI" --kernel walks_matrix --walks-length "$walks_len" -i "$input" -f "$ftype" "${flags[@]}" --compare-json "$baseline"
}

# --- Cases (extend this list as Phase E grows) ---

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
  
echo
echo "[OK] All golden comparisons passed."

