#pragma once

#include <QJsonObject>
#include <QJsonArray>
#include <QTextStream>
#include <QString>

namespace cli
{

    struct CliConfig
    {
        bool verbose = false;

        QString inputPath;
        int fileFormat = 0;
        QString delimiter;
        int twoMode = 0;
        bool hasLabels = false;

        bool computeCentralities = true;
        bool considerWeights = false;
        bool inverseWeights = true;
        bool dropIsolates = false;

        QString dumpJsonPath;
        QString compareJsonPath;

        int benchRuns = 0; // 0 = off
        QString kernel;    // "distance", etc.
        bool strict = false; // if true, timing regressions fail (exit non-zero)

        QString connectivityType = "weak"; // "weak"|"strong", for --kernel connectivity on directed graphs

        QString connMode = "global"; // "local"|"global", for --kernel vertex_connectivity
        int connSource = -1;         // required for connMode == "local"
        int connTarget = -1;         // required for connMode == "local"

        // #281: cross-checks Graph::graphConnectivity() (Esfahanian-Hakimi, O(n+delta^2))
        // against Graph::graphConnectivityNaive() (full O(n^2) pairwise sweep, kept as a
        // trustworthy ground truth) for --kernel vertex_connectivity --conn-mode global. Only
        // meaningful for global mode - graphNodeConnectivity() (local mode) has no naive/fast
        // pair to compare, it's the same single max-flow call either way.
        bool verifyNaive = false;

        qreal katzAlpha = -1; // >= 0 enables Katz Centrality for --kernel prominence

        qreal bonacichAlpha = -1; // >= 0 enables Bonacich Power Centrality for --kernel prominence
        qreal bonacichBeta = 0;

        // "simple_matching"|"jaccard"|"pearson", for --kernel matrix's similarity category.
        // Fix #279: selects which similarity/correlation measure the kernel dumps, so the
        // NaN-guard fix on each measure's degenerate (empty-sample) path can be covered by
        // a dedicated golden baseline instead of only ever exercising simple_matching.
        QString similarityMeasure = "simple_matching";

        // "adjacency"|"distances", for --kernel matrix's similarity category. Selects which
        // Matrix-producing operation feeds similarityMatrix(): the adjacency matrix (AM, the
        // long-standing default - never contains RAND_MAX) or the geodesic distances matrix
        // (DM, which does contain RAND_MAX for unreachable pairs on a disconnected network).
        // similarityMatrix()'s Jaccard branch didn't exclude RAND_MAX from its match/ties
        // count the way distancesMatrix() does, so two actors both unreachable from some
        // third node counted as a false-positive match - only reachable via the "distances"
        // input, hence this flag.
        QString similarityInput = "adjacency";

        // "euclidean"|"manhattan"|"jaccard"|"hamming"|"chebyshev", for --kernel matrix's
        // dissimilarity category (Graph::createMatrixDissimilarities()). Always runs on the
        // adjacency matrix - unlike similarity, dissimilarity has no separate "distances" input
        // mode since it's a numeric-distance measure, not a binary-match one.
        QString dissimilarityMeasure = "euclidean";

        // "single"|"complete"|"average"|"upgma", for --kernel clustering's hierarchical
        // clustering category (Graph::graphClusteringHierarchical()). Selects the linkage
        // method used to compute distances between a newly merged cluster and the remaining
        // clusters. "average" is WPGMA (unweighted mean of the two prior cluster distances);
        // "upgma" weights that mean by each old cluster's member count.
        QString clusteringMethod = "average";

        // "adjacency"|"distances", for --kernel clustering's hierarchical clustering category.
        // Selects the structural-equivalence input matrix fed into graphClusteringHierarchical()
        // (which then derives a dissimilarities matrix from it via dissimilarityMeasure).
        QString clusteringInput = "adjacency";
    };

    // ---------------- printing ----------------

    void printKV(const QString &k, double v);
    void printKV(const QString &k, const QString &v);
    void printKV(const QString &k, int v);
    void printKV(const QString &k, qint64 v);

    // ---------------- deterministic formatting ----------------

    QString d2s(double v);

    // ---------------- JSON I/O ----------------

    bool writeJsonFile(const QString &path, const QJsonObject &obj, QString *err);
    bool readJsonFile(const QString &path, QJsonObject *outObj, QString *err);

    // ---------------- compare helpers ----------------

    bool cmpStr(const QJsonObject &e, const QJsonObject &a, const QString &k, QTextStream &err);
    bool cmpInt(const QJsonObject &e, const QJsonObject &a, const QString &k, QTextStream &err);
    bool cmpBool(const QJsonObject &e, const QJsonObject &a, const QString &k, QTextStream &err);

    bool almostEqual(double a, double b, double rel = 1e-15, double abs = 0.0);

    bool cmpNumStrTol(const QJsonObject &e, const QJsonObject &a,
                      const QString &k, QTextStream &err,
                      double rel = 1e-15, double abs = 0.0);

    bool cmpIntArray(const QJsonArray &e, const QJsonArray &a,
                     QTextStream &err, const QString &what);

    bool cmpStrArray(const QJsonArray &e, const QJsonArray &a,
                     QTextStream &err, const QString &what);

} // namespace cli
