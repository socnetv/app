#!/usr/bin/env python3
"""Compare two labelled runs of bench_cliques.sh: median census time, and whether the saved
census (cliques by size, max size, total) is identical.

Usage: cmp.py <outdir> <labelA> <labelB>
"""
import json
import os
import sys

if len(sys.argv) != 4:
    sys.exit(__doc__)
outdir, a, b = sys.argv[1:]


def load(label):
    rows = {}
    with open(os.path.join(outdir, f"results_{label}.tsv")) as f:
        for line in f:
            name, ms, tot, _ = line.rstrip("\n").split("\t")
            rows[name] = (int(ms), tot)
    return rows


def census(label, name):
    with open(os.path.join(outdir, f"dump_{label}_{name}.json")) as f:
        return json.load(f)["cliques"]


A, B = load(a), load(b)
print(f"{'network':55} {a:>9} {b:>9} {'delta':>7}  {'cliques':>8}  census-identical")
identical = True
for name in A:
    if name not in B:
        continue
    same = census(a, name) == census(b, name)
    identical &= same
    delta = (B[name][0] - A[name][0]) / A[name][0] * 100 if A[name][0] else 0.0
    print(f"{name:55} {A[name][0]:>9} {B[name][0]:>9} {delta:>+6.1f}%  {B[name][1]:>8}  {'YES' if same else 'NO'}")
sys.exit(0 if identical else 1)
