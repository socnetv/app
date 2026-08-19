#!/usr/bin/env bash
set -euo pipefail

# run_report_export_bench.sh — WS16 (#113, CSV report export) baseline/regression tool
#
# Rough before/after measurement tool for Graph::write*() report-writer functions
# (src/graph/reporting/graph_reports.cpp), driven headlessly via --interactive-script (WS12),
# since socnetv-cli's kernels never call these functions (they only ever emit JSON) - this is
# the only headless path that can reach a report writer end-to-end (compute + file I/O).
#
# Covers the matrix-family 'distances' command (Step 1), all 12 centrality/prestige report
# commands (Step 2), and the Step 3 long tail (Reciprocity, standalone Eccentricity, Clustering
# Coefficient, Triad Census) - see the fixtures for the exact list.
#
# Deliberately NOT a CI-threshold-gated regression kernel like run_render_perf_bench.sh's
# EXP_*_MAX_MS baselines - this exists to capture a rough "before" number ahead of the WS16
# CSV-export work, and a comparable "after" number once each step lands. Formal threshold-gating
# can follow later if this becomes a standing regression concern, matching the plan's own
# "rough baselines" framing rather than building out the full apparatus pre-emptively.
#
# Usage:
#   ./scripts/run_report_export_bench.sh
#   REPORT_BENCH_RUNS=11 ./scripts/run_report_export_bench.sh
#   SOCNETV_GUI=./build/SocNetV.app/Contents/MacOS/SocNetV ./scripts/run_report_export_bench.sh

BUILD_TYPE="${BENCH_BUILD_TYPE:-Debug}"  # hint only, matches run_benchmarks.sh's convention
RUNS="${REPORT_BENCH_RUNS:-5}"

if ! [[ "$RUNS" =~ ^[0-9]+$ ]] || (( RUNS < 1 )); then
  echo "ERROR: REPORT_BENCH_RUNS must be a positive integer, got: $RUNS" >&2
  exit 2
fi

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
FIXTURES=(
  "$ROOT_DIR/scripts/fixtures/report_export_bench_small.txt"
  "$ROOT_DIR/scripts/fixtures/report_export_bench_large.txt"
)

# Every report command the fixtures exercise, in the order they appear there. Kept as one list
# so adding a new report command to a fixture only requires adding its name here too.
COMMANDS=(
  distances
  report-centrality-degree
  report-centrality-closeness
  report-centrality-closeness-ir
  report-centrality-betweenness
  report-centrality-stress
  report-centrality-eccentricity
  report-centrality-power
  report-centrality-information
  report-centrality-eigenvector
  report-prestige-degree
  report-prestige-proximity
  report-prestige-pagerank
  report-reciprocity
  report-eccentricity
  report-clustering-coefficient
  report-triad-census
)

# shellcheck source=/dev/null
. "$ROOT_DIR/scripts/lib/find_socnetv_gui.sh"

if [[ -n "${SOCNETV_GUI:-}" ]]; then
  if [[ ! -x "$SOCNETV_GUI" ]]; then
    echo "ERROR: SOCNETV_GUI is set but not executable: $SOCNETV_GUI" >&2
    exit 1
  fi
else
  SOCNETV_GUI="$(find_socnetv_gui "$ROOT_DIR" "$BUILD_TYPE" 2>/dev/null || true)"
fi

if [[ -z "${SOCNETV_GUI:-}" || ! -x "$SOCNETV_GUI" ]]; then
  echo "ERROR: Could not find the SocNetV GUI binary. Build the SocNetV target first." >&2
  echo "Hint: SOCNETV_GUI=/full/path/to/SocNetV $0" >&2
  exit 1
fi

# Prints the median of its integer arguments to stdout. Odd count picks the middle sorted
# value directly; even count averages the two middle sorted values (integer division).
median_of() {
  local -a sorted
  mapfile -t sorted < <(printf '%s\n' "$@" | sort -n)
  local n="${#sorted[@]}"
  local mid=$(( n / 2 ))
  if (( n % 2 == 1 )); then
    echo "${sorted[$mid]}"
  else
    echo $(( (sorted[mid - 1] + sorted[mid]) / 2 ))
  fi
}

echo "INFO: SOCNETV_GUI=$SOCNETV_GUI RUNS=$RUNS" >&2

BENCH_PATTERN="$(IFS='|'; echo "${COMMANDS[*]}")"

for FIXTURE in "${FIXTURES[@]}"; do
  FIXTURE_NAME="$(basename "$FIXTURE" .txt)"

  # One values-array per command, named VALUES_<index> to stay in lockstep with COMMANDS - bash
  # has no associative-array-of-arrays, so this is the simplest portable equivalent.
  for i in "${!COMMANDS[@]}"; do declare -a "VALUES_${i}=()"; done

  for (( run = 1; run <= RUNS; run++ )); do
    RAW_OUTPUT="$(QT_QPA_PLATFORM=offscreen "$SOCNETV_GUI" --interactive-script "$FIXTURE" 2>&1)"
    BENCH_LINES="$(echo "$RAW_OUTPUT" | grep -E "^BENCH (${BENCH_PATTERN}) " || true)"

    if [[ -z "$BENCH_LINES" ]]; then
      echo "ERROR: no BENCH lines produced on run ${run}/${RUNS} for ${FIXTURE_NAME}. Full output:" >&2
      echo "$RAW_OUTPUT" >&2
      exit 1
    fi

    echo "--- ${FIXTURE_NAME} run ${run}/${RUNS} ---" >&2
    echo "$BENCH_LINES" >&2

    for i in "${!COMMANDS[@]}"; do
      CMD="${COMMANDS[$i]}"
      # qInfo()'s stream operator inserts a space after "elapsed_ms=" between the two << operands.
      MS="$(echo "$BENCH_LINES" | grep "^BENCH ${CMD} " | sed -nE 's/.*elapsed_ms= *(-?[0-9]+).*/\1/p')"
      if [[ -z "$MS" ]]; then
        # Not every fixture runs every command (e.g. report-triad-census is O(n^3) and only
        # in the small fixture - impractically slow at the large fixture's N=2000) - skip
        # rather than fail, so one shared COMMANDS list still works across divergent fixtures.
        continue
      fi
      declare -n VALUES_REF="VALUES_${i}"
      VALUES_REF+=("$MS")
    done
  done

  for i in "${!COMMANDS[@]}"; do
    CMD="${COMMANDS[$i]}"
    declare -n VALUES_REF="VALUES_${i}"
    if [[ ${#VALUES_REF[@]} -eq 0 ]]; then
      continue
    fi
    MEDIAN="$(median_of "${VALUES_REF[@]}")"
    echo "MEDIAN ${FIXTURE_NAME} ${CMD} (HTML)=${MEDIAN}ms (of ${RUNS} runs)"
  done

  for i in "${!COMMANDS[@]}"; do unset "VALUES_${i}"; done
done
