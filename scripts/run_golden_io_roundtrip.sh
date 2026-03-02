#!/bin/sh
set -eu

# Runs the io_roundtrip kernel for all JSON baselines in src/tools/baselines/io_roundtrip/*.json.
# Each baseline JSON should contain a "dataset" object with "path" and "filetype
# properties, which are used as input for the io_roundtrip kernel.
# The kernel is run with --compare-json against the baseline, and any failures are reported.
for j in src/tools/baselines/io_roundtrip/*.json; do
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

  echo "==> $j"
  ./build/socnetv-cli --kernel io_roundtrip -i "$path" -f "$ft" --compare-json "$j" || exit 1
done