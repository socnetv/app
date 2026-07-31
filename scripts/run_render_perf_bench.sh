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
#   RENDER_BENCH_BASELINE_SET=macos-m5 ./scripts/run_render_perf_bench.sh
#   SOCNETV_GUI=./build/SocNetV.app/Contents/MacOS/SocNetV ./scripts/run_render_perf_bench.sh
#
# Baseline selection priority:
#   1) RENDER_BENCH_BASELINE=/path/to/render_perf_expected.env
#   2) RENDER_BENCH_BASELINE_SET=<name> (scripts/perf_baselines/<name>/render_perf_expected.env)
#   3) auto: <os>-<arch> (scripts/perf_baselines/<auto>/render_perf_expected.env)

RECORD=0
BUILD_TYPE="${BENCH_BUILD_TYPE:-Debug}"  # hint only, matches run_benchmarks.sh's convention

while [[ $# -gt 0 ]]; do
  case "$1" in
    --record) RECORD=1; shift ;;
    *)
      echo "ERROR: unknown argument: $1" >&2
      exit 2
      ;;
  esac
done

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
    echo "ERROR: expected render-perf file missing: $EXPECTED_FILE" >&2
    echo "Hint: run with --record on the target machine to create it." >&2
    exit 2
  fi
  # shellcheck disable=SC1090
  source "$EXPECTED_FILE"
fi

echo "INFO: SOCNETV_GUI=$SOCNETV_GUI BASELINE_SET=${BASELINE_SET_USED} EXPECTED_FILE=${EXPECTED_FILE} RECORD=${RECORD}" >&2

RAW_OUTPUT="$(QT_QPA_PLATFORM=offscreen "$SOCNETV_GUI" --interactive-script "$FIXTURE" 2>&1)"
BENCH_LINES="$(echo "$RAW_OUTPUT" | grep "BENCH " || true)"

if [[ -z "$BENCH_LINES" ]]; then
  echo "ERROR: no BENCH lines produced. Full output:" >&2
  echo "$RAW_OUTPUT" >&2
  exit 1
fi

echo "$BENCH_LINES"

# BENCH lines appear in fixed script order: render, bulk-node-size, bulk-edge-color, move, render.
declare -a MS_VALUES=()
while IFS= read -r line; do
  # qInfo()'s stream operator inserts a space after "elapsed_ms=" between the two << operands.
  MS_VALUES+=("$(echo "$line" | sed -nE 's/.*elapsed_ms= *(-?[0-9]+).*/\1/p')")
done <<< "$BENCH_LINES"

NAMES=(RENDER_INITIAL BULK_NODE_SIZE BULK_EDGE_COLOR MOVE RENDER_AFTER_MUTATIONS)

if [[ "${RECORD}" == "1" ]]; then
  OUT_DIR="${BASELINE_ROOT}/${BASELINE_SET_USED}"
  mkdir -p "$OUT_DIR"
  OUT_FILE="${OUT_DIR}/render_perf_expected.env"
  {
    echo "# Auto-generated by scripts/run_render_perf_bench.sh --record"
    echo "# BASELINE_SET=${BASELINE_SET_USED}"
    echo "# BASELINE_FILE=scripts/perf_baselines/${BASELINE_SET_USED}/render_perf_expected.env"
    echo "# Thresholds are 2x the measured reference run (upper bound, not exact-value)."
    echo
    for i in "${!NAMES[@]}"; do
      value="${MS_VALUES[$i]:-0}"
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
  value="${MS_VALUES[$i]:-}"
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
    echo "FAIL: $name measured=${value}ms exceeds threshold=${expected}ms"
    FAIL=1
  else
    echo "OK: $name measured=${value}ms within threshold=${expected}ms"
  fi
done

exit "$FAIL"
