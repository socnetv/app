/**
 * @file global.h
 * @brief Global definitions, constants, enumerations, and utility types for SocNetV.
 *
 * All symbols are defined inside the SocNetV namespace.
 * Q_DECLARE_METATYPE registrations are placed outside the namespace,
 * as required by Qt's metatype system.
 */

#ifndef GLOBAL_H
#define GLOBAL_H

#include <QPair>
#include <QMetaType>
#include <QString>

// ============================================================================
// Namespace macros
// ============================================================================

#define SOCNETV_NAMESPACE SocNetV

#ifdef SOCNETV_NAMESPACE
#  define SOCNETV_BEGIN_NAMESPACE  namespace SOCNETV_NAMESPACE {
#  define SOCNETV_END_NAMESPACE    }
#  define SOCNETV_USE_NAMESPACE    using namespace SOCNETV_NAMESPACE;
#else
#  define SOCNETV_BEGIN_NAMESPACE
#  define SOCNETV_END_NAMESPACE
#  define SOCNETV_USE_NAMESPACE
#endif

SOCNETV_BEGIN_NAMESPACE

// ============================================================================
// Version
// ============================================================================

static const QString VERSION = "3.4";

// ============================================================================
// Math constants (define only if not already provided by <cmath>)
// ============================================================================

#ifndef M_PI
static constexpr double M_PI     = 3.14159265358979323846;
#endif

#ifndef M_PI_3
static constexpr double M_PI_3   = 1.04719755119659774615;
#endif

#ifndef M_PI_X_2
static constexpr double M_PI_X_2 = 6.28318530717958647692;
#endif

// ============================================================================
// Enumerations
// ============================================================================

/**
 * @enum NodeShape
 * @brief Possible shapes for nodes in the network visualization.
 */
enum NodeShape {
    Box,
    Circle,
    Diamond,
    Ellipse,
    Triangle,
    Star,
    Person,
    PersonB,
    Bugs,
    Heart,
    Dice,
    Custom
};

/**
 * @enum FileType
 * @brief Supported network file formats.
 */
enum FileType {
    NOT_SAVED          =  0,  ///< New or unsaved/modified network
    GRAPHML            =  1,  ///< .graphml / .xml
    PAJEK              =  2,  ///< .paj / .net
    ADJACENCY          =  3,  ///< .csv / .adj / .sm
    GRAPHVIZ           =  4,  ///< .dot
    UCINET             =  5,  ///< .dl / .dat
    GML                =  6,  ///< .gml
    EDGELIST_WEIGHTED  =  7,  ///< .csv / .txt / .list / .lst / .wlst
    EDGELIST_SIMPLE    =  8,  ///< .csv / .txt / .list / .lst
    TWOMODE            =  9,  ///< .2sm / .aff
    UNRECOGNIZED       = -1   ///< Unrecognised format
};

/**
 * @enum EdgeType
 * @brief Possible edge types in the network.
 */
enum EdgeType {
    Directed     = 0,
    Reciprocated = 1,
    Undirected   = 2
};

/**
 * @enum IndexType
 * @brief Centrality / prestige index identifiers.
 */
enum IndexType {
    DC   =  1,  ///< Degree Centrality
    CC   =  2,  ///< Closeness Centrality
    IRCC =  3,  ///< Influence Range Closeness Centrality
    BC   =  4,  ///< Betweenness Centrality
    SC   =  5,  ///< Stress Centrality
    EC   =  6,  ///< Eccentricity Centrality
    PC   =  7,  ///< Power Centrality
    IC   =  8,  ///< Information Centrality
    EVC  =  9,  ///< Eigenvector Centrality
    DP   = 10,  ///< Degree Prestige
    PRP  = 11,  ///< PageRank Prestige
    PP   = 12   ///< Proximity Prestige
};

/**
 * @enum ChartType
 * @brief Chart style for prominence distribution visualizations.
 */
enum ChartType {
    None   = -1,
    Spline =  0,
    Area   =  1,
    Bars   =  2
};

/**
 * @enum NetworkRequestType
 * @brief Identifies the purpose of an outgoing network request.
 */
enum NetworkRequestType {
    Generic     = 0,
    Crawler     = 1,
    CheckUpdate = 2
};

// ============================================================================
// User-message severity constants
// ============================================================================

static const int USER_MSG_INFO                 = 0;
static const int USER_MSG_CRITICAL             = 1;
static const int USER_MSG_CRITICAL_NO_NETWORK  = 2;
static const int USER_MSG_CRITICAL_NO_EDGES    = 3;
static const int USER_MSG_QUESTION             = 4;
static const int USER_MSG_QUESTION_CUSTOM      = 5;

// ============================================================================
// Subgraph type constants
// ============================================================================

static const int SUBGRAPH_CLIQUE = 1;
static const int SUBGRAPH_STAR   = 2;
static const int SUBGRAPH_CYCLE  = 3;
static const int SUBGRAPH_LINE   = 4;

// ============================================================================
// Matrix type constants
// ============================================================================

static const int MATRIX_ADJACENCY             =  1;
static const int MATRIX_DISTANCES             =  2;
static const int MATRIX_DEGREE                =  3;
static const int MATRIX_LAPLACIAN             =  4;
static const int MATRIX_ADJACENCY_INVERSE     =  5;
static const int MATRIX_GEODESICS             =  6;
static const int MATRIX_REACHABILITY          =  7;
static const int MATRIX_ADJACENCY_TRANSPOSE   =  8;
static const int MATRIX_COCITATION            =  9;
static const int MATRIX_DISTANCES_EUCLIDEAN   = 12;
static const int MATRIX_DISTANCES_MANHATTAN   = 13;
static const int MATRIX_DISTANCES_JACCARD     = 14;
static const int MATRIX_DISTANCES_HAMMING     = 15;
static const int MATRIX_DISTANCES_CHEBYSHEV   = 16;

// ============================================================================
// Structs and value types
// ============================================================================

/**
 * @struct ClickedEdge
 * @brief Carries the identity and type of a clicked edge.
 */
struct ClickedEdge {
    int v1   = 0;  ///< First vertex
    int v2   = 0;  ///< Second vertex
    int type = 0;  ///< Edge type
};

/**
 * @typedef SelectedEdge
 * @brief Identifies a selected edge by its two endpoint vertex numbers.
 *
 * Defined here (not in graphicswidget.h) so that Graph and GraphicsWidget
 * can both use it without a circular include dependency.
 */
typedef QPair<int, int> SelectedEdge;

/**
 * @class MyEdge
 * @brief Lightweight value type representing a directed or undirected edge.
 */
class MyEdge {
public:
    int    source  = 0;
    int    target  = 0;
    double weight  = 0.0;
    int    type    = 0;
    double rWeight = 0.0;  ///< Reverse / reciprocal weight

    MyEdge() = default;

    MyEdge(const int &from, const int &to,
           const double &w  = 0.0,
           const int   &t   = 0,
           const double &rw = 0.0)
        : source(from), target(to), weight(w), type(t), rWeight(rw) {}
};

/**
 * @class GraphDistance
 * @brief Holds a (target, distance) pair for use in Dijkstra's priority queue.
 */
class GraphDistance {
public:
    int target;
    int distance;

    GraphDistance(int t, int dist)
        : target(t), distance(dist) {}
};

/**
 * @class GraphDistancesCompare
 * @brief Min-priority comparator for GraphDistance (used in std::priority_queue).
 */
class GraphDistancesCompare {
public:
    bool operator()(const GraphDistance &t1, const GraphDistance &t2) const {
        if (t1.distance == t2.distance)
            return t1.target > t2.target;
        return t1.distance > t2.distance;
    }
};

/**
 * @class PairVF
 * @brief (value, frequency) pair, used in distribution charts.
 */
class PairVF {
public:
    qreal value;
    qreal frequency;

    PairVF(qreal v, qreal f)
        : value(v), frequency(f) {}
};

/**
 * @class PairVFCompare
 * @brief Min-priority comparator for PairVF (used in std::priority_queue).
 */
class PairVFCompare {
public:
    bool operator()(const PairVF &v1, const PairVF &v2) const {
        return v1.value > v2.value;
    }
};

SOCNETV_END_NAMESPACE

// ============================================================================
// Qt metatype registrations
// Must live outside the SocNetV namespace.
// ============================================================================

Q_DECLARE_METATYPE(SOCNETV_NAMESPACE::MyEdge)
Q_DECLARE_METATYPE(SOCNETV_NAMESPACE::NetworkRequestType)
Q_DECLARE_METATYPE(SOCNETV_NAMESPACE::SelectedEdge)

#endif // GLOBAL_H