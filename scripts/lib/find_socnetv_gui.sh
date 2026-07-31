#!/bin/sh
# POSIX helper for locating the SocNetV GUI binary (macOS/Linux), analogous to
# find_socnetv_cli.sh. Binary name is platform-specific per CMakeLists.txt: a macOS .app
# bundle, "SocNetV" on Windows, lowercase "socnetv" everywhere else (Linux).
# Usage:
#   . "$ROOT_DIR/scripts/lib/find_socnetv_gui.sh"
#   GUI="$(find_socnetv_gui "$ROOT_DIR" "$BUILD_TYPE")" || exit 1

find_socnetv_gui() {
  root=$1
  build_type=$2

  for p in \
    "$root/build/SocNetV.app/Contents/MacOS/SocNetV" \
    "$root/build/socnetv" \
    "$root/build/SocNetV" \
    "$root/build/$build_type/socnetv" \
    "$root/builds/__unspec__/$build_type/socnetv" \
    "$root/builds/__unspec__/Debug/socnetv"
  do
    if [ -x "$p" ]; then
      echo "$p"
      return 0
    fi
  done

  if [ -d "$root/builds" ]; then
    for t in "$build_type" Debug Release; do
      p=$(find "$root/builds" -maxdepth 3 -type f -name socnetv -path "*/$t/*" 2>/dev/null | sed -n '1p')
      if [ -n "${p:-}" ] && [ -x "$p" ]; then
        echo "$p"
        return 0
      fi
    done
  fi

  return 1
}
