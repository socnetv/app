#!/usr/bin/env bash
# =============================================================================
# update_translations.sh — SocNetV Translation Update Helper
# =============================================================================
# Location: app/scripts/update_translations.sh
#
# Usage:
#   ./update_translations.sh [--src <source_dir>] [--ts <ts_file> ...]
#
# Examples:
#   cd app/scripts && ./update_translations.sh
#   ./update_translations.sh --src ../src/ --ts ../translations/socnetv_de.ts
#
# What it does:
#   1. Finds lupdate in PATH or Qt installation (macOS/Linux)
#   2. Ensures every .ts file has a proper language= attribute in its header
#      (lupdate silently skips files without one)
#   3. Runs lupdate to pull in new/changed strings from source
#   4. Reports next steps
# =============================================================================

set -euo pipefail

# --- Resolve app root relative to this script --------------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
APP_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

# --- Defaults ----------------------------------------------------------------
SRC_DIR="$APP_DIR/src"
TRANSLATIONS_DIR="$APP_DIR/translations"
TS_FILES=()

# --- Argument parsing --------------------------------------------------------
while [[ $# -gt 0 ]]; do
    case "$1" in
        --src)  SRC_DIR="$2";       shift 2 ;;
        --ts)   TS_FILES+=("$2");   shift 2 ;;
        -h|--help)
            grep '^#' "$0" | grep -v '#!/' | sed 's/^# \{0,1\}//'
            exit 0 ;;
        *) echo "ERROR: Unknown option: $1"; exit 1 ;;
    esac
done

# --- Sanity checks -----------------------------------------------------------
if [[ ! -d "$SRC_DIR" ]]; then
    echo "ERROR: Source directory not found: $SRC_DIR"
    echo "  Override with: --src <path>"
    exit 1
fi

if [[ ! -d "$TRANSLATIONS_DIR" ]]; then
    echo "ERROR: Translations directory not found: $TRANSLATIONS_DIR"
    exit 1
fi

# --- Locate lupdate ----------------------------------------------------------
find_lupdate() {
    if command -v lupdate &>/dev/null; then
        echo "lupdate"; return
    fi
    if [[ "$OSTYPE" == "darwin"* ]]; then
        local found
        found=$(find "$HOME/Qt" -type f -name "lupdate" 2>/dev/null | sort -Vr | head -1)
        [[ -n "$found" ]] && echo "$found" && return
    fi
    for dir in /usr/lib/qt6/bin \
               /usr/lib/x86_64-linux-gnu/qt6/bin \
               /opt/Qt/*/gcc_64/bin \
               /usr/local/Qt/*/gcc_64/bin; do
        [[ -x "$dir/lupdate" ]] && echo "$dir/lupdate" && return
    done
    echo ""
}

LUPDATE=$(find_lupdate)
if [[ -z "$LUPDATE" ]]; then
    echo "ERROR: lupdate not found."
    echo "  macOS: install Qt via https://www.qt.io/download"
    echo "  Linux: sudo apt install qt6-tools-dev-tools  # or distro equivalent"
    exit 1
fi

echo "lupdate : $LUPDATE"
echo "version : $("$LUPDATE" -version 2>&1 | head -1)"
echo "app dir : $APP_DIR"
echo "sources : $SRC_DIR"
echo

# --- Auto-detect .ts files if none given -------------------------------------
if [[ ${#TS_FILES[@]} -eq 0 ]]; then
    while IFS= read -r f; do
        TS_FILES+=("$f")
    done < <(find "$TRANSLATIONS_DIR" -name "*.ts" | sort)
fi

if [[ ${#TS_FILES[@]} -eq 0 ]]; then
    echo "ERROR: No .ts files found in $TRANSLATIONS_DIR"
    echo "  Use --ts <file> to specify manually."
    exit 1
fi

echo "Translation files:"
printf '  %s\n' "${TS_FILES[@]}"
echo

# --- Fix missing language= attribute -----------------------------------------
# lupdate silently skips .ts files whose <TS> element has no language= attr.
# We derive the code from the filename: socnetv_de.ts -> "de"

fix_language_attribute() {
    local tsfile="$1"
    local lang_code
    lang_code=$(basename "$tsfile" .ts | sed 's/.*_//')

    if grep -q '<TS[^>]*language=' "$tsfile"; then
        echo "  [ok]     $(basename "$tsfile")  (language=\"$lang_code\")"
        return
    fi

    echo "  [fixing] $(basename "$tsfile")  — adding language=\"$lang_code\""
    if [[ "$OSTYPE" == "darwin"* ]]; then
        sed -i '' "s|<TS version=\"\([^\"]*\)\">|<TS version=\"\1\" language=\"${lang_code}\">|" "$tsfile"
    else
        sed -i "s|<TS version=\"\([^\"]*\)\">|<TS version=\"\1\" language=\"${lang_code}\">|" "$tsfile"
    fi
}

echo "Checking language attributes..."
for ts in "${TS_FILES[@]}"; do
    [[ -f "$ts" ]] || { echo "  [warn]   $ts — not found, skipping"; continue; }
    fix_language_attribute "$ts"
done
echo

# --- Run lupdate -------------------------------------------------------------
echo "Running lupdate..."
echo "------------------------------------------------------"
"$LUPDATE" "$SRC_DIR" -ts "${TS_FILES[@]}"
echo "------------------------------------------------------"
echo

# --- Next steps --------------------------------------------------------------
LRELEASE="${LUPDATE/lupdate/lrelease}"
echo "Done. Next steps:"
echo "  1. Translate new/unfinished strings in Qt Linguist:"
echo "       linguist ${TS_FILES[*]}"
echo
echo "  2. Compile to .qm (or just rebuild via CMake):"
if [[ -x "$LRELEASE" ]]; then
    echo "       $LRELEASE ${TS_FILES[*]}"
else
    echo "       lrelease ${TS_FILES[*]}"
fi