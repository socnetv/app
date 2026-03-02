#!/usr/bin/env bash
set -uo pipefail

# Runs the io_roundtrip kernel for all datasets shipped in our src/data, using the filetype inferred from the extension.
# The kernel is run without --compare-json

for f in $(find src/data -type f); do
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
  ./build/socnetv-cli --kernel io_roundtrip -i "$f" -f "$ft" || exit 1
done

