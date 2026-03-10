#!/bin/sh
set -eu

# Runs the io_roundtrip kernel for all JSON baselines in src/tools/baselines/io_roundtrip/*.json.
# Default: compares against each baseline.
# With --update: regenerates each baseline JSON in-place from current code.

UPDATE=0
KEEP_BAK=1

ROOT_DIR=$(cd "$(dirname "$0")/.." && pwd)
BUILD_TYPE=${BUILD_TYPE:-Debug}

. "$ROOT_DIR/scripts/lib/find_socnetv_cli.sh"

if [ -n "${SOCNETV_CLI:-}" ]; then
  CLI="$SOCNETV_CLI"
else
  CLI=$(find_socnetv_cli "$ROOT_DIR" "$BUILD_TYPE" 2>/dev/null || true)
fi

if [ -z "${CLI:-}" ] || [ ! -x "$CLI" ]; then
  echo "[ERROR] socnetv-cli not found/executable." >&2
  echo "Hint: SOCNETV_CLI=/full/path/to/socnetv-cli $0" >&2
  exit 2
fi

echo "[io] Using CLI: $CLI"


usage() {
  echo "Usage: $0 [--update] [--no-bak]" >&2
  exit 2
}

while [ $# -gt 0 ]; do
  case "$1" in
    --update) UPDATE=1 ;;
    --no-bak) KEEP_BAK=0 ;;
    -h|--help) usage ;;
    *) echo "Unknown arg: $1" >&2; usage ;;
  esac
  shift
done

for j in src/tools/baselines/io_roundtrip/*.json; do
  echo "json filename: $j"
  [ -f "$j" ] || continue

  # Extract dataset.path (a JSON string) and dataset.filetype (a JSON number)
  path=$(
    tr -d '\n' < "$j" \
    | sed -n 's/.*"dataset"[[:space:]]*:[[:space:]]*{[^}]*"path"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p'
  )

  ft=$(
    tr -d '\n' < "$j" \
    | sed -n 's/.*"dataset"[[:space:]]*:[[:space:]]*{[^}]*"filetype"[[:space:]]*:[[:space:]]*\([-0-9][0-9]*\).*/\1/p'
  )

  [ -n "$path" ] || { echo "ERROR: dataset.path not found in $j" >&2; exit 1; }
  [ -n "$ft" ]   || { echo "ERROR: dataset.filetype not found in $j" >&2; exit 1; }
  [ -f "$path" ] || { echo "ERROR: dataset.path '$path' not found on disk (baseline $j)" >&2; exit 1; }

  echo "==> $j  (dataset=$path FT=$ft)"

  if [ "$UPDATE" -eq 1 ]; then
    tmp="$(mktemp "${j}.tmp.XXXXXX")"
    # Generate fresh JSON and overwrite baseline
    "$CLI"  --kernel io_roundtrip -i "$path" -f "$ft" --dump-json "$tmp" >/dev/null

    if [ "$KEEP_BAK" -eq 1 ]; then
      cp -f "$j" "${j}.bak"
    fi

    mv -f "$tmp" "$j"
    echo "UPDATED: $j"
  else
    "$CLI" --kernel io_roundtrip -i "$path" -f "$ft" --compare-json "$j" || exit 1
  fi
done