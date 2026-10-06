#!/usr/bin/env bash
set -euo pipefail

# Times the maximal-clique census (CLIQUES_MS, reported by the clustering kernel) on dense
# random networks and a few real ones, and saves each run's census JSON so two builds can be
# compared for identical results as well as speed (see cmp.py).
#
# Usage:
#   ./scripts/max-clique/bench_cliques.sh <socnetv-cli> <label> [outdir]
#
#   <label>   names this run's outputs: <outdir>/results_<label>.tsv, dump_<label>_<net>.json
#   [outdir]  default: ${TMPDIR:-/tmp}/socnetv-clique-bench
#
# Environment:
#   RUNS             repetitions per network, median is reported (default 5)
#   LARGE_NETS_DIR   directory holding the two large real networks (default
#                    ${HOME}/socnetv/library/nets/large); cases whose file is missing are skipped
#
# Compare two labelled runs:  ./scripts/max-clique/cmp.py <outdir> <labelA> <labelB>
#
# Use the same build type for both runs, and don't build or run anything heavy meanwhile.

CLI="${1:?usage: bench_cliques.sh <socnetv-cli> <label> [outdir]}"
LABEL="${2:?usage: bench_cliques.sh <socnetv-cli> <label> [outdir]}"
OUTDIR="${3:-${TMPDIR:-/tmp}/socnetv-clique-bench}"
RUNS="${RUNS:-5}"

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$HERE/../.." && pwd)"
DATA="$ROOT_DIR/src/data"
NETS="$HERE/nets"
LARGE_NETS_DIR="${LARGE_NETS_DIR:-${HOME}/socnetv/library/nets/large}"

[[ -x "$CLI" ]] || { echo "ERROR: not executable: $CLI" >&2; exit 1; }
mkdir -p "$OUTDIR"
OUT="$OUTDIR/results_$LABEL.tsv"
: > "$OUT"

# path:format  (format enum: 1 GraphML, 2 Pajek, 5 UCINET DL)
cases=(
  "$NETS/ER_N40_p50.net:2"  "$NETS/ER_N60_p50.net:2"  "$NETS/ER_N80_p50.net:2"
  "$NETS/ER_N100_p50.net:2" "$NETS/ER_N150_p50.net:2"
  "$NETS/ER_N60_p70.net:2"  "$NETS/ER_N80_p70.net:2"  "$NETS/ER_N100_p70.net:2"
  "$NETS/ER_N40_p85.net:2"  "$NETS/ER_N60_p85.net:2"
  "$DATA/Zachary_Karate_Club.dl:5"
  "$DATA/Stokman_Ziegler_Corporate_Interlocks_Netherlands.dl:5"
  "$LARGE_NETS_DIR/Random_ER_Undir_N300_E1735.graphml:1"
  "$LARGE_NETS_DIR/Random_ER_Undir_N1000_E19879.graphml:1"
)

for c in "${cases[@]}"; do
  f="${c%%:*}"; ft="${c##*:}"; name="$(basename "$f")"
  if [[ ! -f "$f" ]]; then
    echo "[skip] $name (not found)" >&2
    continue
  fi
  dump="$OUTDIR/dump_${LABEL}_$name.json"
  ms=()
  for _ in $(seq "$RUNS"); do
    # perl alarm = 10 min watchdog; macOS has no timeout(1).
    v="$(perl -e 'alarm 600; exec @ARGV' "$CLI" --kernel clustering -i "$f" -f "$ft" \
          -w 0 -x 1 -k 0 -j "$dump" 2>&1 | sed -n 's/^CLIQUES_MS=//p')"
    ms+=("$v")
  done
  med="$(printf '%s\n' "${ms[@]}" | sort -n | sed -n "$(( (RUNS + 1) / 2 ))p")"
  tot="$(python3 -c "import json,sys;print(json.load(open(sys.argv[1]))['cliques']['total_cliques'])" "$dump" 2>/dev/null || echo "?")"
  printf '%s\t%s\t%s\t(%s)\n' "$name" "$med" "$tot" "${ms[*]}" | tee -a "$OUT"
done
echo "[bench] wrote $OUT" >&2
