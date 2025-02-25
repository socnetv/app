/**
 * @file global.h
 * @brief This file contains global definitions, constants, and utility classes for the SocNetV application.
 */

#ifndef GLOBAL_H
#define GLOBAL_H

#include <QMetaType>

#define SOCNETV_NAMESPACE SocNetV

#ifdef SOCNETV_NAMESPACE
#  define SOCNETV_BEGIN_NAMESPACE namespace SOCNETV_NAMESPACE {
#  define SOCNETV_END_NAMESPACE }
#  define SOCNETV_USE_NAMESPACE using namespace SOCNETV_NAMESPACE;
#else
#  define SOCNETV_BEGIN_NAMESPACE
#  define SOCNETV_END_NAMESPACE
#  define SOCNETV_USE_NAMESPACE
#endif

SOCNETV_BEGIN_NAMESPACE

#ifndef M_PI_3
#define M_PI_3 (1.04719755119659774615)
#endif

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

#ifndef M_PI_X_2
#define M_PI_X_2 (6.28318530717958647692)
#endif

static const QString VERSION="3.2";

/**
 * @enum NodeShape
 * @brief Enumeration of possible shapes for nodes in the network.
 */
enum NodeShape{
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
 * @brief Enumeration of possible file types for network data.
 */
enum FileType {
    NOT_SAVED         = 0,  ///< New network not saved yet or modified network
    GRAPHML           = 1,  ///< .GRAPHML .XML
    PAJEK             = 2,  ///< .PAJ .NET
    ADJACENCY         = 3,  ///< .CSV .ADJ .SM
    GRAPHVIZ          = 4,  ///< .DOT
    UCINET            = 5,  ///< .DL .DAT
    GML               = 6,  ///< .GML
    EDGELIST_WEIGHTED = 7,  ///< .CSV, .TXT, .LIST, LST, WLST
    EDGELIST_SIMPLE   = 8,  ///< .CSV, .TXT, .LIST, LST
    TWOMODE           = 9,  ///< .2SM .AFF
    UNRECOGNIZED      =-1   ///< Unrecognized file format
};

/**
 * @enum EdgeType
 * @brief Enumeration of possible edge types in the network.
 */
enum EdgeType {
    Directed = 0,
    Reciprocated = 1,
    Undirected = 2
};

/**
 * @enum IndexType
 * @brief Enumeration of possible index types for network analysis.
 */
enum IndexType {
    DC   = 1,
    CC   = 2,
    IRCC = 3,
    BC   = 4,
    SC   = 5,
    EC   = 6,
    PC   = 7,
    IC   = 8,
    EVC  = 9,
    DP   = 10,
    PRP  = 11,
    PP   = 12
};

/**
 * @enum ChartType
 * @brief Enumeration of possible chart types for visualizing network data.
 */
enum ChartType {
    None = -1,
    Spline = 0,
    Area = 1,
    Bars = 2
};

/**
 * @enum NetworkRequestType
 * @brief Enumeration of possible network request types.
 */
enum NetworkRequestType {
    Generic = 0,
    Crawler = 1,
    CheckUpdate = 2
};

static const int USER_MSG_INFO=0;
static const int USER_MSG_CRITICAL=1;
static const int USER_MSG_CRITICAL_NO_NETWORK=2;
static const int USER_MSG_CRITICAL_NO_EDGES=3;
static const int USER_MSG_QUESTION=4;
static const int USER_MSG_QUESTION_CUSTOM=5;

static const int SUBGRAPH_CLIQUE = 1;
static const int SUBGRAPH_STAR   = 2;
static const int SUBGRAPH_CYCLE  = 3;
static const int SUBGRAPH_LINE   = 4;

static const int MATRIX_ADJACENCY        = 1;
static const int MATRIX_DISTANCES        = 2;
static const int MATRIX_DEGREE           = 3;
static const int MATRIX_LAPLACIAN        = 4;
static const int MATRIX_ADJACENCY_INVERSE = 5;
static const int MATRIX_GEODESICS        = 6;
static const int MATRIX_REACHABILITY     = 7;
static const int MATRIX_ADJACENCY_TRANSPOSE = 8;
static const int MATRIX_COCITATION = 9;
static const int MATRIX_DISTANCES_EUCLIDEAN = 12;
static const int MATRIX_DISTANCES_MANHATTAN= 13;
static const int MATRIX_DISTANCES_JACCARD= 14;
static const int MATRIX_DISTANCES_HAMMING= 15;
static const int MATRIX_DISTANCES_CHEBYSHEV= 16;

/**
 * @struct ClickedEdge
 * @brief Structure to hold information about a clicked edge in the network.
 */
struct ClickedEdge {
    int v1;  ///< First vertex of the edge
    int v2;  ///< Second vertex of the edge
    int type; ///< Type of the edge
};

typedef QPair<int, int> SelectedEdge;

/**
 * @class MyEdge
 * @brief Class representing an edge in the network.
 */
class MyEdge {
public:
    int source;  ///< Source vertex of the edge
    int target;  ///< Target vertex of the edge
    double weight;  ///< Weight of the edge
    int type;  ///< Type of the edge
    double rWeight;  ///< Reserved weight of the edge

    MyEdge();  ///< Default constructor
    MyEdge(const int &from, const int &to, const double &w =0, const int &type=0, const double &rw = 0);  ///< Parameterized constructor
    MyEdge(const MyEdge &edge);  ///< Copy constructor
    ~MyEdge();  ///< Destructor
};

/**
 * @class GraphDistance
 * @brief Holds the distance to target. Used in Graph::dijkstra() priority_queue.
 */
class GraphDistance
{
public:
    int target;  ///< Target vertex
    int distance;  ///< Distance to the target vertex

    GraphDistance(int t, int dist);  ///< Constructor
};

/**
 * @class GraphDistancesCompare
 * @brief Metric to implement a min-priority queue.
 * The operator returns true if t1 is closer than t2.
 * Used in Graph::dijkstra() priority_queue.
 */
class GraphDistancesCompare {
public:
    bool operator()(GraphDistance& t1, GraphDistance& t2);  ///< Comparison operator
};

/**
 * @class PairVF
 * @brief Class representing a pair of value and frequency.
 */
class PairVF
{
public:
    qreal value;  ///< Value
    qreal frequency;  ///< Frequency

    PairVF(qreal v, qreal f);  ///< Constructor
};

/**
 * @class PairVFCompare
 * @brief Implements a min-priority queue.
 */
class PairVFCompare {
public:
    bool operator()(PairVF& v1, PairVF& v2);  ///< Comparison operator
};

SOCNETV_END_NAMESPACE

Q_DECLARE_METATYPE(SOCNETV_NAMESPACE::MyEdge)
Q_DECLARE_METATYPE(SOCNETV_NAMESPACE::NetworkRequestType)

#endif // GLOBAL_H
