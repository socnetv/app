#!/usr/bin/env bash
set -euo pipefail

# run_report_export_bench.sh — WS16 (#113, CSV report export) Step 0 baseline
#
# Rough before/after measurement tool for Graph::write*() report-writer functions
# (src/graph/reporting/graph_reports.cpp), driven headlessly via --interactive-script (WS12),
# since socnetv-cli's kernels never call these functions (they only ever emit JSON) - this is
# the only headless path that can reach a report writer end-to-end (compute + file I/O).
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

for FIXTURE in "${FIXTURES[@]}"; do
  FIXTURE_NAME="$(basename "$FIXTURE" .txt)"
  declare -a DISTANCES_VALUES=() CENTRALITY_VALUES=()

  for (( run = 1; run <= RUNS; run++ )); do
    RAW_OUTPUT="$(QT_QPA_PLATFORM=offscreen "$SOCNETV_GUI" --interactive-script "$FIXTURE" 2>&1)"
    BENCH_LINES="$(echo "$RAW_OUTPUT" | grep -E "^BENCH (distances|report-centrality-degree) " || true)"

    if [[ -z "$BENCH_LINES" ]]; then
      echo "ERROR: no BENCH lines produced on run ${run}/${RUNS} for ${FIXTURE_NAME}. Full output:" >&2
      echo "$RAW_OUTPUT" >&2
      exit 1
    fi

    echo "--- ${FIXTURE_NAME} run ${run}/${RUNS} ---" >&2
    echo "$BENCH_LINES" >&2

    # qInfo()'s stream operator inserts a space after "elapsed_ms=" between the two << operands.
    DISTANCES_MS="$(echo "$BENCH_LINES" | grep "^BENCH distances " | sed -nE 's/.*elapsed_ms= *(-?[0-9]+).*/\1/p')"
    CENTRALITY_MS="$(echo "$BENCH_LINES" | grep "^BENCH report-centrality-degree " | sed -nE 's/.*elapsed_ms= *(-?[0-9]+).*/\1/p')"

    if [[ -z "$DISTANCES_MS" || -z "$CENTRALITY_MS" ]]; then
      echo "ERROR: missing expected BENCH line(s) on run ${run}/${RUNS} for ${FIXTURE_NAME}." >&2
      exit 1
    fi

    DISTANCES_VALUES+=("$DISTANCES_MS")
    CENTRALITY_VALUES+=("$CENTRALITY_MS")
  done

  DISTANCES_MEDIAN="$(median_of "${DISTANCES_VALUES[@]}")"
  CENTRALITY_MEDIAN="$(median_of "${CENTRALITY_VALUES[@]}")"

  echo "MEDIAN ${FIXTURE_NAME} distances (writeMatrix, HTML)=${DISTANCES_MEDIAN}ms (of ${RUNS} runs)"
  echo "MEDIAN ${FIXTURE_NAME} report-centrality-degree (writeCentralityDegree, HTML)=${CENTRALITY_MEDIAN}ms (of ${RUNS} runs)"

  unset DISTANCES_VALUES CENTRALITY_VALUES
done
