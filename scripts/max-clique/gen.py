#!/usr/bin/env python3
"""Generate the seeded dense random test networks (Pajek .net) used by bench_cliques.sh.

Usage: gen.py [outdir]     (default: <this dir>/nets)

Each network is G(n, p): every vertex pair is tied with probability p. The seed is fixed, so
re-running reproduces the committed files byte for byte.
"""
import os
import random
import sys

SEED = 20261006
# (n, p): the census cost grows with the number of maximal cliques, so density matters more than n.
CONFIGS = [(40, 0.5), (60, 0.5), (80, 0.5), (100, 0.5), (150, 0.5),
           (60, 0.7), (80, 0.7), (100, 0.7), (40, 0.85), (60, 0.85)]


def gen(n, p, path):
    rng = random.Random(SEED)
    edges = [(i, j) for i in range(1, n + 1) for j in range(i + 1, n + 1) if rng.random() < p]
    with open(path, "w") as f:
        f.write(f"*Vertices {n}\n")
        for i in range(1, n + 1):
            f.write(f'{i} "{i}"\n')
        f.write("*Edges\n")
        for a, b in edges:
            f.write(f"{a} {b} 1\n")
    print(path, n, len(edges))


if __name__ == "__main__":
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(os.path.abspath(__file__)), "nets")
    os.makedirs(out, exist_ok=True)
    for n, p in CONFIGS:
        gen(n, p, os.path.join(out, f"ER_N{n}_p{int(p * 100)}.net"))
