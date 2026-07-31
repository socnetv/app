/**
 * @file matrix_storage_bench.cpp
 * @brief Standalone WS5 A2.0 benchmark: QHash<int, QPair<int,qreal>> (current APSP storage,
 *        one per GraphVertex) vs. a flat Matrix (proposed APSP storage), measuring construction
 *        and lookup speed for a single (N, topology, structure) configuration per run. Peak RSS
 *        is measured externally by the driver script (see scripts/run_matrix_storage_bench.sh),
 *        not by this process itself — see the note above buildComponents() for why.
 * @copyright
 *   Copyright (C) 2005-2026 by Dimitris B. Kalamaras.
 *   This file is part of SocNetV (Social Network Visualizer).
 * @license
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, version 3 or later.
 *   For more details, see <http://www.gnu.org/licenses/>.
 * @see https://socnetv.org
 */

#include "../matrix.h"

#include <QElapsedTimer>
#include <QHash>
#include <QPair>
#include <QString>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace {

// Deliberately no in-process RSS measurement here. It was tried (both mach_task_basic_info and
// getrusage's ru_maxrss) and both under-report for this tool's shape: a tight, single-threaded,
// syscall-free compute loop doesn't generate enough kernel bookkeeping events (page faults,
// scheduling activity) for ru_maxrss to be updated in real time, so a read taken mid-run lags
// well behind the true peak — confirmed by comparing against macOS's own `/usr/bin/time -l`
// (which reads the accurate parent-side wait4() rusage after the process exits) at N=20000: the
// in-process reading undercounted by 6x-14x depending on the exact measurement used. Memory is
// measured externally instead — see scripts/run_matrix_storage_bench.sh, which wraps each
// invocation in the platform's equivalent of `time -l`/`time -v` and reports the peak RSS that
// way. This tool only reports timing and a checksum.

struct Component {
    int start;
    int size; // 0-size "components" are isolates
};

// Builds the component structure for one of three topologies:
//   "connected"    — a single component covering all N vertices.
//   "disconnected" — ~8 roughly-equal-sized fully-connected components plus ~5% isolates.
//   "giant"        — one giant component (90% of N) plus a long tail of halving-sized
//                    components down to isolates, approximating the component-size
//                    distribution of real-world networks (one dominant component, a long
//                    tail of small ones) rather than "disconnected"'s equal-sized split.
std::vector<Component> buildComponents(int n, const QString &topology)
{
    std::vector<Component> components;
    if (topology == "connected" || n < 16) {
        components.push_back({0, n});
        return components;
    }

    if (topology == "giant") {
        const int giantSize = (n * 9) / 10; // 90%
        components.push_back({0, giantSize});
        int pos = giantSize;
        int remaining = n - giantSize;
        while (remaining > 8) {
            const int half = remaining / 2;
            components.push_back({pos, half});
            pos += half;
            remaining -= half;
        }
        for (int i = 0; i < remaining; ++i) {
            components.push_back({pos, 1});
            ++pos;
        }
        return components;
    }

    // "disconnected"
    const int isolateCount = std::max(1, n / 20); // ~5%
    const int remaining = n - isolateCount;
    const int numGroups = 8;
    const int groupSize = remaining / numGroups;

    int pos = 0;
    for (int g = 0; g < numGroups; ++g) {
        const int size = (g == numGroups - 1) ? (remaining - pos) : groupSize;
        components.push_back({pos, size});
        pos += size;
    }
    for (int i = 0; i < isolateCount; ++i) {
        components.push_back({pos, 1});
        ++pos;
    }
    return components;
}

qreal sampleValue(int i, int j)
{
    return 1.0 + static_cast<qreal>((i + j) % 7);
}

} // namespace

int main(int argc, char *argv[])
{
    int n = -1;
    QString topology;
    QString structure;

    for (int i = 1; i < argc; ++i) {
        const QString arg = QString::fromLocal8Bit(argv[i]);
        if (arg == "--n" && i + 1 < argc) {
            n = QString::fromLocal8Bit(argv[++i]).toInt();
        } else if (arg == "--topology" && i + 1 < argc) {
            topology = QString::fromLocal8Bit(argv[++i]);
        } else if (arg == "--structure" && i + 1 < argc) {
            structure = QString::fromLocal8Bit(argv[++i]);
        }
    }

    if (n <= 0
        || (topology != "connected" && topology != "disconnected" && topology != "giant")
        || (structure != "qhash" && structure != "matrix")) {
        fprintf(stderr,
                "usage: matrix-storage-bench --n <N> --topology <connected|disconnected|giant> "
                "--structure <qhash|matrix>\n");
        return 1;
    }

    const std::vector<Component> components = buildComponents(n, topology);

    // Map each vertex to its component's [start, start+size) range for O(1) "same component?"
    // checks during construction and lookup.
    std::vector<int> compStart(n), compEnd(n);
    for (const Component &c : components) {
        for (int v = c.start; v < c.start + c.size; ++v) {
            compStart[v] = c.start;
            compEnd[v] = c.start + c.size;
        }
    }

    QElapsedTimer timer;
    qreal checksum = 0.0;
    qint64 constructMs = 0;
    qint64 lookupMs = 0;

    if (structure == "qhash") {
        std::vector<QHash<int, QPair<int, qreal>>> perVertex(n);

        timer.start();
        for (int i = 0; i < n; ++i) {
            perVertex[i].reserve(compEnd[i] - compStart[i]);
            for (int j = compStart[i]; j < compEnd[i]; ++j) {
                if (j == i) continue;
                perVertex[i].insert(j, qMakePair(0, sampleValue(i, j)));
            }
        }
        constructMs = timer.elapsed();

        timer.restart();
        const int curRelation = 0;
        for (int i = 0; i < n; ++i) {
            const auto &hash = perVertex[i];
            for (int j = 0; j < n; ++j) {
                if (j == i) continue;
                auto it = hash.constFind(j);
                while (it != hash.constEnd() && it.key() == j) {
                    if (it.value().first == curRelation) {
                        checksum += it.value().second;
                        break;
                    }
                    ++it;
                }
            }
        }
        lookupMs = timer.elapsed();

    } else { // matrix
        timer.start();
        Matrix m(n, n);
        for (int i = 0; i < n; ++i) {
            for (int j = compStart[i]; j < compEnd[i]; ++j) {
                if (j == i) continue;
                m.setItem(i, j, sampleValue(i, j));
            }
        }
        constructMs = timer.elapsed();

        timer.restart();
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                if (j == i) continue;
                checksum += m.item(i, j);
            }
        }
        lookupMs = timer.elapsed();
    }

    printf("%d,%s,%s,%lld,%lld,%.3f\n",
           n, qPrintable(topology), qPrintable(structure),
           static_cast<long long>(constructMs), static_cast<long long>(lookupMs), checksum);

    return 0;
}
