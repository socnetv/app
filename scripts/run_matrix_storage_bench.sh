#!/usr/bin/env bash
# run_matrix_storage_bench.sh — WS5 A2.0 empirical validation driver
#
# Runs the standalone matrix-storage-bench tool once per (N, topology, structure)
# configuration — one process per run, so each measurement starts from a clean process.
# Wraps each run in the platform's time tool to get an accurate peak-RSS reading: the
# tool's own in-process attempts at this (mach_task_basic_info, getrusage) were found to
# under-report by 6x-14x for this tool's tight compute-bound shape (see the comment above
# buildComponents() in src/tools/matrix_storage_bench.cpp) — only the parent-side wait4()
# view that `time -l`/`time -v` use is accurate here.
#
# Usage:
#   cmake -S . -B build -DBUILD_MATRIX_BENCH=ON ...
#   cmake --build build --target matrix-storage-bench -j$(nproc)
#   ./scripts/run_matrix_storage_bench.sh
#   MATRIX_BENCH=./build/matrix-storage-bench ./scripts/run_matrix_storage_bench.sh
set -uo pipefail

BENCH="${MATRIX_BENCH:-./build/matrix-storage-bench}"

if [[ ! -x "$BENCH" ]]; then
    echo "error: $BENCH not found or not executable." >&2
    echo "Build it first: cmake -S . -B build -DBUILD_MATRIX_BENCH=ON && cmake --build build --target matrix-storage-bench" >&2
    exit 1
fi

PLATFORM="$(uname)"
HAVE_GNU_TIME_V=0
if [[ "$PLATFORM" != "Darwin" ]] && /usr/bin/time -v true >/dev/null 2>&1; then
    HAVE_GNU_TIME_V=1
fi

run_one() {
    local n="$1" topology="$2" structure="$3"
    local stderr_file csv_line peak_rss_bytes

    stderr_file="$(mktemp)"

    if [[ "$PLATFORM" == "Darwin" ]]; then
        csv_line="$(/usr/bin/time -l "$BENCH" --n "$n" --topology "$topology" --structure "$structure" 2>"$stderr_file")"
        peak_rss_bytes="$(awk '/maximum resident set size/ {print $1}' "$stderr_file")"
    elif [[ "$HAVE_GNU_TIME_V" -eq 1 ]]; then
        csv_line="$(/usr/bin/time -v "$BENCH" --n "$n" --topology "$topology" --structure "$structure" 2>"$stderr_file")"
        local peak_rss_kb
        peak_rss_kb="$(awk -F': ' '/Maximum resident set size/ {print $2}' "$stderr_file")"
        peak_rss_bytes="$((peak_rss_kb * 1024))"
    else
        csv_line="$("$BENCH" --n "$n" --topology "$topology" --structure "$structure")"
        peak_rss_bytes="NA"
    fi

    rm -f "$stderr_file"
    echo "${csv_line},${peak_rss_bytes}"
}

echo "n,topology,structure,construct_ms,lookup_ms,checksum,peak_rss_bytes"

for n in 100 1000 7343; do
    for topology in connected disconnected giant; do
        for structure in qhash matrix; do
            run_one "$n" "$topology" "$structure"
        done
    done
done
