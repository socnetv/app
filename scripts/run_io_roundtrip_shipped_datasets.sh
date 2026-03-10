#!/usr/bin/env bash
set -uo pipefail

# Runs the io_roundtrip kernel for all datasets shipped in src/data, using the filetype inferred from the extension.
# The kernel is run without --compare-json

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_TYPE="${BUILD_TYPE:-Debug}"

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

echo "[io_all] Using CLI: $CLI"

# Safe iteration over filenames
while IFS= read -r -d '' f; do
  case "$f" in
    *.graphml) ft=1 ;;
    *.paj|*.net) ft=2 ;;
    *.adj) ft=3 ;;
    *.dot) ft=4 ;;
    *.dl) ft=5 ;;
    *.gml) ft=6 ;;
    *.wlst) ft=7 ;;
    *.lst) ft=8 ;;
    *) continue ;;
  esac

  echo "=== IO_ROUNDTRIP $f (FT$ft) ==="
  "$CLI" --kernel io_roundtrip -i "$f" -f "$ft" || exit 1
done < <(find "$ROOT_DIR/src/data" -type f -print0)