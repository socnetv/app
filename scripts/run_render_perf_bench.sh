#!/usr/bin/env bash
set -euo pipefail

# run_render_perf_bench.sh — WS10/WS6.6 canvas rendering-performance regression kernel (#240)
#
# Drives the real SocNetV GUI binary through --interactive-script (WS12) with
# QT_QPA_PLATFORM=offscreen (no visible window, but the real QGraphicsScene/QGraphicsView
# paint/geometry code path - not a reimplementation). Compares each BENCH line's elapsed_ms
# against an upper-bound threshold: "must be faster than X ms", not exact equality, since
# GUI-driven timing has more run-to-run variance than the headless compute kernels.
#
# Runs the full fixture RENDER_BENCH_RUNS times (default 7) and uses the per-metric MEDIAN
# across runs, both when recording and when comparing - a single-shot measurement was found to
# vary run-to-run by ~25% on the recording machine, which made both the recorded threshold and
# any comparison against it noisy in either direction. Matches run_benchmarks.sh's median-of-N
# convention for the headless compute kernels (there via the CLI's own --bench flag; here via an
# external loop, since each run launches a fresh GUI process).
#
# Why offscreen, and why its numbers differ from a real on-screen session (measured, not
# assumed): on the recording machine, the `render` command is ~2x-2.4x FASTER offscreen than
# on-screen (no compositor/backing-store round trip) - but non-paint bulk operations
# (bulk-node-size, bulk-edge-color) measure the same in both modes. So these thresholds are
# only meaningful compared against other offscreen runs, not against a real user's on-screen
# experience; they exist to catch regressions in the underlying paint/geometry code, not to
# report perceived performance. See roadmap_ws6_testing_ci_regression.md's WS6.6 section for
# the full numbers.
#
# Usage:
#   ./scripts/run_render_perf_bench.sh
#   ./scripts/run_render_perf_bench.sh --record
#   RENDER_BENCH_RUNS=11 ./scripts/run_render_perf_bench.sh --record
#   RENDER_BENCH_BASELINE_SET=macos-m5 ./scripts/run_render_perf_bench.sh
#   SOCNETV_GUI=./build/SocNetV.app/Contents/MacOS/SocNetV ./scripts/run_render_perf_bench.sh
#
# Baseline selection priority:
#   1) RENDER_BENCH_BASELINE=/path/to/render_perf_expected.env
#   2) RENDER_BENCH_BASELINE_SET=<name> (scripts/perf_baselines/<name>/render_perf_expected.env)
#   3) auto: <os>-<arch> (scripts/perf_baselines/<auto>/render_perf_expected.env)

RECORD=0
BUILD_TYPE="${BENCH_BUILD_TYPE:-Debug}"  # hint only, matches run_benchmarks.sh's convention
RUNS="${RENDER_BENCH_RUNS:-7}"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --record) RECORD=1; shift ;;
    *)
      echo "ERROR: unknown argument: $1" >&2
      exit 2
      ;;
  esac
done

if ! [[ "$RUNS" =~ ^[0-9]+$ ]] || (( RUNS < 1 )); then
  echo "ERROR: RENDER_BENCH_RUNS must be a positive integer, got: $RUNS" >&2
  exit 2
fi

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
FIXTURE="$ROOT_DIR/scripts/fixtures/render_perf_script.txt"
BASELINE_ROOT="$ROOT_DIR/scripts/perf_baselines"

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

sanitize_id() {
  echo "$1" | tr -cd '[:alnum:]._-' | sed 's/^\.*//; s/\.*$//'
}

auto_baseline_set() {
  local os arch
  os="$(uname -s 2>/dev/null || echo unknownOS)"
  arch="$(uname -m 2>/dev/null || echo unknownARCH)"
  case "$os" in
    Darwin) os="macos" ;;
    Linux)  os="linux" ;;
    *)      os="$(echo "$os" | tr '[:upper:]' '[:lower:]')" ;;
  esac
  sanitize_id "${os}-${arch}"
}

resolve_expected_file() {
  if [[ -n "${RENDER_BENCH_BASELINE:-}" ]]; then
    echo "${RENDER_BENCH_BASELINE}"
    return 0
  fi
  local set_name
  set_name="$(sanitize_id "${RENDER_BENCH_BASELINE_SET:-$(auto_baseline_set)}")"
  echo "${BASELINE_ROOT}/${set_name}/render_perf_expected.env"
}

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

EXPECTED_FILE="$(resolve_expected_file)"
RESOLVED_SET="$(sanitize_id "${RENDER_BENCH_BASELINE_SET:-$(auto_baseline_set)}")"
BASELINE_SET_USED="${RESOLVED_SET}"
[[ -n "${RENDER_BENCH_BASELINE:-}" ]] && BASELINE_SET_USED="custom"

if [[ "${RECORD}" == "1" && -n "${RENDER_BENCH_BASELINE_SET:-}" && "${RENDER_BENCH_RECORD_ALLOW_OVERRIDE:-0}" != "1" ]]; then
  AUTO_SET="$(auto_baseline_set)"
  if [[ "${RESOLVED_SET}" != "${AUTO_SET}" ]]; then
    echo "ERROR: refusing to --record into RENDER_BENCH_BASELINE_SET=${RENDER_BENCH_BASELINE_SET} (auto=${AUTO_SET})." >&2
    echo "Run on the target machine, or set RENDER_BENCH_RECORD_ALLOW_OVERRIDE=1 to force." >&2
    exit 2
  fi
fi

if [[ "${RECORD}" != "1" ]]; then
  if [[ ! -f "$EXPECTED_FILE" ]]; then
    echo "INFO: no render-perf baseline for this machine (${EXPECTED_FILE} not found)." >&2
    echo "Hint: run with --record on this machine to create one. Continuing with all metrics SKIPPED." >&2
  else
    # shellcheck disable=SC1090
    source "$EXPECTED_FILE"
  fi
fi

echo "INFO: SOCNETV_GUI=$SOCNETV_GUI BASELINE_SET=${BASELINE_SET_USED} EXPECTED_FILE=${EXPECTED_FILE} RECORD=${RECORD} RUNS=${RUNS}" >&2

NAMES=(RENDER_INITIAL BULK_NODE_SIZE BULK_EDGE_COLOR MOVE RENDER_AFTER_MUTATIONS)
declare -a RUN_VALUES_0=() RUN_VALUES_1=() RUN_VALUES_2=() RUN_VALUES_3=() RUN_VALUES_4=()

for (( run = 1; run <= RUNS; run++ )); do
  RAW_OUTPUT="$(QT_QPA_PLATFORM=offscreen "$SOCNETV_GUI" --interactive-script "$FIXTURE" 2>&1)"
  # Allowlist match on the metric commands only - every interactive-script command logs its own
  # "BENCH <name> ..." line (uniform logging added after this kernel first shipped), so a bare
  # "BENCH " grep would also pick up setup/teardown commands (erdos-m, delay, quit, ...) and
  # shift every downstream value out of alignment. Matching by name is robust against the
  # fixture script gaining more setup commands later without needing this filter updated again.
  BENCH_LINES="$(echo "$RAW_OUTPUT" | grep -E "^BENCH (render|bulk-node-size|bulk-edge-color|move) " || true)"

  if [[ -z "$BENCH_LINES" ]]; then
    echo "ERROR: no BENCH lines produced on run ${run}/${RUNS}. Full output:" >&2
    echo "$RAW_OUTPUT" >&2
    exit 1
  fi

  echo "--- run ${run}/${RUNS} ---" >&2
  echo "$BENCH_LINES" >&2

  # BENCH lines appear in fixed script order: render, bulk-node-size, bulk-edge-color, move, render.
  declare -a MS_VALUES=()
  while IFS= read -r line; do
    # qInfo()'s stream operator inserts a space after "elapsed_ms=" between the two << operands.
    MS_VALUES+=("$(echo "$line" | sed -nE 's/.*elapsed_ms= *(-?[0-9]+).*/\1/p')")
  done <<< "$BENCH_LINES"

  if [[ "${#MS_VALUES[@]}" -ne "${#NAMES[@]}" ]]; then
    echo "ERROR: expected ${#NAMES[@]} BENCH lines on run ${run}/${RUNS}, got ${#MS_VALUES[@]}." >&2
    exit 1
  fi

  RUN_VALUES_0+=("${MS_VALUES[0]}")
  RUN_VALUES_1+=("${MS_VALUES[1]}")
  RUN_VALUES_2+=("${MS_VALUES[2]}")
  RUN_VALUES_3+=("${MS_VALUES[3]}")
  RUN_VALUES_4+=("${MS_VALUES[4]}")
done

MEDIANS=(
  "$(median_of "${RUN_VALUES_0[@]}")"
  "$(median_of "${RUN_VALUES_1[@]}")"
  "$(median_of "${RUN_VALUES_2[@]}")"
  "$(median_of "${RUN_VALUES_3[@]}")"
  "$(median_of "${RUN_VALUES_4[@]}")"
)

for i in "${!NAMES[@]}"; do
  echo "MEDIAN ${NAMES[$i]}=${MEDIANS[$i]}ms (of ${RUNS} runs)"
done

if [[ "${RECORD}" == "1" ]]; then
  OUT_DIR="${BASELINE_ROOT}/${BASELINE_SET_USED}"
  mkdir -p "$OUT_DIR"
  OUT_FILE="${OUT_DIR}/render_perf_expected.env"
  {
    echo "# Auto-generated by scripts/run_render_perf_bench.sh --record"
    echo "# BASELINE_SET=${BASELINE_SET_USED}"
    echo "# BASELINE_FILE=scripts/perf_baselines/${BASELINE_SET_USED}/render_perf_expected.env"
    echo "# Thresholds are 2x the median-of-${RUNS} measured reference run (upper bound, not exact-value)."
    echo
    for i in "${!NAMES[@]}"; do
      value="${MEDIANS[$i]:-0}"
      threshold=$(( value * 2 ))
      # Floor at 15ms: some operations (e.g. a single-vertex "move") measure near-instant, and
      # doubling a 0-1ms reading would produce a threshold too tight to survive normal jitter.
      if (( threshold < 15 )); then
        threshold=15
      fi
      echo "EXP_${NAMES[$i]}_MAX_MS=${threshold}"
    done
  } > "$OUT_FILE"
  echo "Recorded: $OUT_FILE"
  exit 0
fi

FAIL=0
for i in "${!NAMES[@]}"; do
  name="${NAMES[$i]}"
  value="${MEDIANS[$i]:-}"
  var="EXP_${name}_MAX_MS"
  expected="${!var:-}"
  if [[ -z "$expected" ]]; then
    echo "SKIP: $name (no baseline for $var)"
    continue
  fi
  if [[ -z "$value" ]]; then
    echo "FAIL: $name - no measurement captured"
    FAIL=1
    continue
  fi
  if (( value > expected )); then
    echo "FAIL: $name median=${value}ms exceeds threshold=${expected}ms"
    FAIL=1
  else
    echo "OK: $name median=${value}ms within threshold=${expected}ms"
  fi
done

exit "$FAIL"
