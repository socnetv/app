#!/usr/bin/env bash
set -euo pipefail

# SocNetV performance micro-benchmark runner (macOS/Linux)
#
# Usage:
#   ./scripts/run_benchmarks.sh
#   ./scripts/run_benchmarks.sh --strict
#   SOCNETV_CLI=./build/socnetv-cli ./scripts/run_benchmarks.sh

STRICT=0
if [[ "${1:-}" == "--strict" ]]; then
  STRICT=1
fi

# Root = repo root (script lives in scripts/)
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

DEFAULT_CLI="$ROOT_DIR/build/socnetv-cli"
if [[ ! -x "$DEFAULT_CLI" ]]; then
  DEFAULT_CLI="$ROOT_DIR/builds/__unspec__/Debug/socnetv-cli"
fi
SOCNETV_CLI="${SOCNETV_CLI:-$DEFAULT_CLI}"

PERF_EXPECTED_FILE="${PERF_EXPECTED_FILE:-$ROOT_DIR/scripts/perf_expected.env}"

if [[ ! -x "$SOCNETV_CLI" ]]; then
  echo "ERROR: socnetv-cli not found or not executable: $SOCNETV_CLI" >&2
  echo "Set SOCNETV_CLI=/path/to/socnetv-cli" >&2
  exit 2
fi

if [[ ! -f "$PERF_EXPECTED_FILE" ]]; then
  echo "ERROR: expected perf file missing: $PERF_EXPECTED_FILE" >&2
  exit 2
fi

# shellcheck disable=SC1090
source "$PERF_EXPECTED_FILE"

# ---------------- helpers ----------------

extract_kv_int() {
  local key="$1"
  awk -F= -v k="$key" '$1==k { print $2 }'
}

pct_slower_than() {
  # returns 0 if actual <= expected * 1.10
  local actual="$1"
  local expected="$2"

  [[ -z "$actual" || -z "$expected" ]] && return 1

  if (( 100 * actual > 110 * expected )); then
    return 1
  fi
  return 0
}

run_case() {
  local tag="$1"
  shift

  echo "=== $tag ==="
  local out
  out="$("$SOCNETV_CLI" "$@" 2>/dev/null || true)"

  local ok median
  ok="$(printf '%s\n' "$out" | extract_kv_int OK | tail -n1)"
  median="$(printf '%s\n' "$out" | extract_kv_int COMPUTE_MS_MEDIAN | tail -n1)"

  echo "OK=$ok MEDIAN_MS=$median"

  if [[ "$ok" != "1" || -z "$median" ]]; then
    echo "ERROR: benchmark did not produce valid output for $tag" >&2
    return 2
  fi

  local expected_var="EXP_${tag}_MEDIAN_MS"
  local expected="${!expected_var:-}"

  if [[ -z "$expected" ]]; then
    echo "WARN: no expected median configured for $tag"
    return 0
  fi

  if pct_slower_than "$median" "$expected"; then
    echo "OK: ${median}ms <= ${expected}ms (+10%)"
    return 0
  else
    echo "WARN: ${median}ms > ${expected}ms (+10%)"
    [[ "$STRICT" == "1" ]] && return 1
    return 0
  fi
}

fail=0

# ---------------- benchmark cases ----------------

# Medium (UCINET time_1)
run_case "EIES48_T1_C1_W1" \
  -i "$ROOT_DIR/src/data/Freeman_EIES_network_48actors_Acquaintanceship_at_time_1.dl" \
  -f 5 -c 1 -w 1 -x 1 -k 0 --bench 20 || fail=1

# Medium (UCINET time_2)
run_case "EIES48_T2_C1_W1" \
  -i "$ROOT_DIR/src/data/Freeman_EIES_network_48actors_Acquaintanceship_at_time_2.dl" \
  -f 5 -c 1 -w 1 -x 1 -k 0 --bench 20 || fail=1

# Large BA (500, m=3), centralities ON
run_case "BA500_M3_C1_W0" \
  -i "$ROOT_DIR/src/data/Benchmark_BA_Directed_N500_m3.paj" \
  -f 2 -c 1 -w 0 -x 1 -k 0 --bench 20 || fail=1

# Large BA (500, m=3), distances only
run_case "BA500_M3_C0_W0" \
  -i "$ROOT_DIR/src/data/Benchmark_BA_Directed_N500_m3.paj" \
  -f 2 -c 0 -w 0 -x 1 -k 0 --bench 20 || fail=1

echo "=== DONE ==="
if [[ "$fail" == "1" ]]; then
  echo "RESULT=FAIL"
  exit 1
fi

echo "RESULT=OK"
exit 0
