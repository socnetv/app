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


enum FileType {
    NOT_SAVED         = 0,  //  New network not saved yet or modified network
    GRAPHML           = 1,  // .GRAPHML .XML
    PAJEK             = 2,  // .PAJ .NET
    ADJACENCY         = 3,  // .CSV .ADJ .SM
    GRAPHVIZ          = 4,  // .DOT
    UCINET            = 5,  // .DL .DAT
    GML               = 6,  // .GML
    EDGELIST_WEIGHTED = 7,  // .CSV, .TXT, .LIST, LST, WLST
    EDGELIST_SIMPLE   = 8,  // .CSV, .TXT, .LIST, LST
    TWOMODE           = 9,  // .2SM .AFF
    UNRECOGNIZED      =-1  // UNRECOGNIZED FILE FORMAT
};


enum EdgeType {
    Directed = 0,
    Reciprocated = 1,
    Undirected = 2
};


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


enum ChartType {
    None = -1,
    Spline = 0,
    Area = 1,
    Bars = 2
};

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






struct ClickedEdge {
    int v1;
    int v2;
    int type;
};



typedef QPair<int, int> SelectedEdge;


class MyEdge {
public:
    int source;
    int target;
    double weight;
    int type;
    double rWeight;
    MyEdge() { source=0; target=0;weight=0;type=0; rWeight=0; }
    MyEdge (const int &from, const int &to, const double &w =0, const int &type=0, const double &rw = 0)
        : source(from), target(to), weight(w), type(type), rWeight(rw)  {  }
    // Copy constructor
    MyEdge (const MyEdge &edge) {
        source = edge.source;
        target = edge.target;
        weight = edge.weight;
        rWeight = edge.rWeight ;
        type = edge.type;
    }
    ~MyEdge(){}
};


/**
 * @brief Holds the distance to target. Used in Graph::dijkstra() priority_queue
 */
class GraphDistance
{
public:
    int target;
    int distance;

    GraphDistance(int t, int dist)
        : target(t), distance(dist)
    {

    }
};



/**
 * @brief Metric to implement a min-priority queue.
 * The operator returns true if if t1 is closer than t2
 * Used in Graph::dijkstra() priority_queue
 */
class GraphDistancesCompare {
public:
    bool operator()(GraphDistance& t1, GraphDistance& t2)
    {
        if (t1.distance == t2.distance)
            return t1.target > t2.target;
        return t1.distance > t2.distance;  //minimum priority
    }
};




class PairVF
{
public:
    qreal value;
    qreal frequency;

    PairVF(qreal v, qreal f)
        : value(v), frequency(f)  { }
};


// implement a min-priority queue
class PairVFCompare {
public:
    bool operator()(PairVF& v1, PairVF& v2)
    {
        return v1.value > v2.value; //minimum priority
        // Returns true if t1 is closer than t2
        // else
    }
};



SOCNETV_END_NAMESPACE


Q_DECLARE_METATYPE(SOCNETV_NAMESPACE::MyEdge)
Q_DECLARE_METATYPE(SOCNETV_NAMESPACE::NetworkRequestType)


#endif // GLOBAL_H
