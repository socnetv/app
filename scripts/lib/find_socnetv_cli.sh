#!/bin/sh
# POSIX helper for locating socnetv-cli (macOS/Linux).
# Usage:
#   . "$ROOT_DIR/scripts/lib/find_socnetv_cli.sh"
#   CLI="$(find_socnetv_cli "$ROOT_DIR" "$BUILD_TYPE")" || exit 1

find_socnetv_cli() {
  root=$1
  build_type=$2

  # Fast-path known layouts
  for p in \
    "$root/build/socnetv-cli" \
    "$root/build/$build_type/socnetv-cli" \
    "$root/builds/__unspec__/$build_type/socnetv-cli" \
    "$root/builds/__unspec__/Debug/socnetv-cli"
  do
    if [ -x "$p" ]; then
      echo "$p"
      return 0
    fi
  done

  # QtCreator kits / custom build dirs:
  # builds/<kit>/<Debug|Release>/socnetv-cli
  if [ -d "$root/builds" ]; then
    for t in "$build_type" Debug Release; do
      p=$(find "$root/builds" -maxdepth 3 -type f -name socnetv-cli -path "*/$t/*" 2>/dev/null | sed -n '1p')
      if [ -n "${p:-}" ] && [ -x "$p" ]; then
        echo "$p"
        return 0
      fi
    done
  fi

  return 1
}