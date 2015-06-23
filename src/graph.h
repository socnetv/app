/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 1.9
 Written in Qt
 
                         graph.h  -  description
                             -------------------
    copyright            : (C) 2005-2015 by Dimitris B. Kalamaras
    email                : dimitris.kalamaras@gmail.com
    website:             : http://dimitris.apeiro.gr
    project site         : http://socnetv.sourceforge.net
 ***************************************************************************/

/*******************************************************************************
*     This program is free software: you can redistribute it and/or modify     *
*     it under the terms of the GNU General Public License as published by     *
*     the Free Software Foundation, either version 3 of the License, or        *
*     (at your option) any later version.                                      *
*                                                                              *
*     This program is distributed in the hope that it will be useful,          *
*     but WITHOUT ANY WARRANTY; without even the implied warranty of           *
*     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
*     GNU General Public License for more details.                             *
*                                                                              *
*     You should have received a copy of the GNU General Public License        *
*     along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
********************************************************************************/

#ifndef GRAPH_H
#define GRAPH_H


#include <QObject>
#include <QDateTime> 	// used in exporting centrality files
#include <QList>
#include <QHash>
#include <QTextStream>
#include <QThread>
//FYI: stack is a wrapper around <deque> in C++, see: www.cplusplus.com/reference/stl/stack
#include <stack>
#include <map>

#include "vertex.h"
#include "matrix.h"
#include "parser.h"
#include "webcrawler.h"

using namespace std;


class QPointF;


/**	This is the main class for a Graph, used in conjuction with Vertex, Parser and Matrix objects.

    Graph class has the interface and the various network algorithms
    Vertex class holds each vertex data (colors, strings, statistics, etc)
    Matrix class holds the adjacency matrix of the network.
    Parser class loads files of networks.
*/

typedef QList<Vertex*> Vertices;

typedef QHash <QString, int> H_StrToInt;
typedef QHash <long int, long int> H_Int;

typedef QPair <float, bool> pair_f_b;
typedef QPair <int, pair_f_b > rel_w_bool;
typedef QHash < int, rel_w_bool > H_edges;
typedef QHash<QString, bool> H_StrToBool;




class Distance
{
public:
    int target;
    int distance;

    Distance(int t, int dist)
        : target(t), distance(dist)
    {

    }
};

struct Distance1 {
    int target;
    int distance;
};

// implement a min-priority queue
class CompareDistances {
    public:
    bool operator()(Distance& t1, Distance& t2)
    {
       if (t1.distance == t2.distance)
            return t1.target > t2.target;
       return t1.distance > t2.distance;  //minimum priority
       // Returns true if t1 is closer than t2
       // else
    }
};






class Graph:  public QObject{
    Q_OBJECT
    QThread file_parserThread;
    QThread wc_parserThread;
    QThread wc_spiderThread;
public slots:
    int currentRelation();

    /** Slots to signals from Parser */

    void addRelationFromParser(QString);
    void createVertex(	int i, int size, QString nodeColor,
                        QString numColor, int numSize,
                        QString label, QString lColor, int lSize,
                        QPointF p, QString nodeShape, bool signalMW
                        );//Main vertex creation call

    void setFileType(int, QString, int,int, bool);
    void removeDummyNode(int);
    void terminateParserThreads (QString reason);

    /** Slots to signals from GraphicsWidget and Parser*/
    void createEdge (int, int, float, QString, int, bool, bool);	//GW and Parser.
    void createEdge (int, int, float, int, bool, bool);		//GW
    void createEdgeWebCrawler (int, int);					//WebCrawler
    void nodeMovement(bool state, int type, int cW, int cH);		//Called by MW to start movement

    void slotSetEdgeVisibility(int relation, int, int, bool);

    //auxiliary createVertex functions
    void createVertex(int i, QPointF p); 				//Called by GW
    void createVertex(int i, int canvasWidth, int canvasHeight); 	//Called by MW
    void createVertexWebCrawler(QString label, int i) ;

    /** Slots to signals from MainWindow */
    void changeRelation(int);
    void addRelationFromUser(QString relation);
    void setCanvasDimensions(int w, int h);
    void filterIsolateVertices ( bool );		//Called by MW to filter orphan vertices
    void filterEdgesByWeight (float, bool);		//Called by MW to filter edges over/under a weight
    void filterEdgesByRelation(int relation, bool status);

    void webCrawl(QString, int, int, bool extLinks, bool intLinks);	//Called by MW to start a web crawler...
    void setGraphChanged(bool changed) { graphModified = changed; }


signals:
    /** Signals to MainWindow */
    void updateProgressDialog(int );
    void graphChanged();					//call to update MW widgets

    void signalFileType (int, QString, int,int, bool);	//notifies MW what we have loaded.
    void statusMessage (QString message);			//updates statusbar message
    void addRelationToMW(QString newRelation);
    void describeDataset(QString);
    void signalNodeSizesByOutDegree(bool);
    void signalNodeSizesByInDegree(bool);

    /** Signals to GraphicsWidget */
    void drawNode( int ,int,  QString, QString, int, QString, QString, int,
                   QPointF, QString, bool, bool, bool);	//call GW to draw a node

    void eraseNode (long int);						//erase node from GW
    void drawEdge(int, int, float, bool, bool, QString, bool);	//call GW to draw an edge
    void eraseEdge(int, int);					//emited from removeEdge() to GW to clear the edge item.
    void setEdgeVisibility (int, int, int, bool);			// emitted from each Vertex
    void setVertexVisibility(long int, bool);		//notifies GW to disable a node
    void setNodeSize(long int, int);
    void setNodeShape(const long int, const QString);
    void setNodeColor(long int, QString);
    void setNodeLabel(long int, QString);
    void drawEdgeReciprocal(int, int);				//call GW to draw the edge as symmetric one
    void changeEdgeColor(long int, long int, QString);
    void addGuideCircle(int, int, int);				//call GW to draw a circular layout line somewhere.
    void addGuideHLine (int);					//call GW to draw a horizontal layout line somewhere.
    void moveNode(int, qreal, qreal);

    /** Signals to Vertice */
    void relationChanged(int);

    /** Signals to Crawler threads */
    void  operateSpider();


public: 	
    /* INIT AND CLEAR*/
    Graph();
    void clear();
    ~Graph();			//destroy object

    void setSocNetV_Version (QString ver) { VERSION = ver; }

    void setShowLabels(bool toggle);
    void setShowNumbersInsideNodes(bool toggle);

    /*FILES (READ AND WRITE)*/
    bool loadGraph (const QString, const QString m_codecName,
                    const bool,
                    const int maxWidth,
                    const int maxHeight,
                    const int format,
                    const int two_sm_mode);

    bool saveGraph( QString fileName, int fileType,
                    QString networkName, int maxWidth, int maxHeight
                    );
    bool saveGraphToPajekFormat (QString fileName,QString networkName,  int maxWidth, int maxHeight);
    bool saveGraphToAdjacencyFormat (QString fileName);
    bool saveGraphToDotFormat (QString fileName, QString networkName, int maxWidth, int maxHeight);
    bool saveGraphToGraphMLFormat (QString fileName,QString networkName,  int maxWidth, int maxHeight);

    /* VERTICES */
    int lastVertexNumber();
    int firstVertexNumber();

    int hasVertex(long int );
    int hasVertex(QString);
    void removeVertex (long int );

    void setInitVertexSize (const long int);
    void setVertexSize(const long int &v, const int &newsize );
    void setAllVerticesSize(const int &newsize);
    int vertexSize(const long int &v);

    void setInitVertexShape (const QString);
    void setVertexShape(const int v, const QString shape);
    void setAllVerticesShape(const QString shape);
    QString vertexShape(const int &v);

    void setInitVertexColor (const QString &color);
    void setVertexColor(const long &v, const QString &color);
    void setAllVerticesColor(const QString &color);
    QColor vertexColor(const long int &v);

    void setInitVertexNumberColor ( QString color);
    void setInitVertexNumberSize (int size);

    void setInitVertexLabelSize(int newSize);
    void setVertexLabelSize(int v, int newSize);

    void setInitVertexLabelColor(QString color);
    void setVertexLabel(int v, QString label);
    void setVertexLabelColor(int v1, QString color);
    QString vertexLabel(const long int &v1);

    void updateVertCoords(int v, int x, int y);

    int vertices(const bool dropIsolates=false, const bool countAll=false) ;

    int outboundEdges (int i) ;
    int inboundEdges (int i) ;

    int outDegree(int);
    int inDegree(int);

    int verticesWithOutboundEdges();
    int verticesWithInboundEdges();
    int verticesWithReciprocalEdges();

    QList<int> verticesIsolated();

    qreal euclideian_distance(const QPointF &a, const QPointF &b);
    qreal euclideian_distance(const QPointF &a);
    int sign(const qreal &D);

    qreal layoutForceDirected_F_rep(const qreal &dist,const qreal &optimalDistance) ;
    qreal layoutForceDirected_F_att(const qreal &dist,const qreal &optimalDistance) ;

    void layoutForceDirected_Eades_moveNodes(const qreal &c4);
    void layoutForceDirected_FR_moveNodes(const qreal &temperature) ;

    qreal computeOptimalDistance(const int &Vertices);
    void compute_angles( const QPointF &Delta,
                         const qreal &dist,
                         qreal &angle1,
                         qreal &angle2,
                         qreal &degrees1,
                         qreal &degrees2 );

    /* EDGES */
    int enabledEdges();
    void edges();
    float hasArc (const long &v1, const long &v2);
    bool hasEdge (const int &v1, const long int &v2);
    void removeEdge (int v1, int v2);

    bool isWeighted();

    void setArcWeight (const long int &v1, const long int &v2, const float &w);
    void setInitEdgeColor(const QString &);

    void setEdgeColor(const long int &v1, const long int &v2, const QString &color);
    QString edgeColor (const long int &v1, const long int &v2);
    bool setAllEdgesColor(const QString &color);

    float density();

    bool symmetricEdge(int v1, int v2);
    bool isSymmetric();
    void symmetrize();

    void createAdjacencyMatrix(const bool dropIsolates=false,
                               const bool considerWeights=true,
                               const bool inverseWeights=false,
                               const bool symmetrize=false );
    bool invertAdjacencyMatrix(const QString &method);


    /* PRINT OUT TO FILES*/

    void writeDataSetToFile(const QString dir, const QString );
    void writeAdjacencyMatrixTo(QTextStream& os);
    void writeAdjacencyMatrix(const QString, const char*);

    void writeInvertAdjacencyMatrix(const QString &filename,
                                    const QString &,
                                    const QString &);
    void writeDistanceMatrix(const QString fn, const char*,
                             const bool considerWeights,
                             const bool inverseWeights,
                             const bool dropIsolates);
    void writeNumberOfGeodesicsMatrix(const QString fn, const char*,
                                      const bool considerWeights,
                                      const bool inverseWeights);
    void writeEccentricity(const QString, const bool considerWeights,
                           const bool inverseWeights, const bool dropIsolates);

 //   friend QTextStream& operator <<  (QTextStream& os, Graph& m);

    void writeCentralityDegree(const QString,
                               const bool weights,
                               const bool dropIsolates);
    void writeCentralityCloseness(const QString,
                                  const bool weights,
                                  const bool inverseWeights,
                                  const bool dropIsolates);
    void writeCentralityClosenessInfluenceRange(const QString,
                                                const bool weights,
                                                const bool inverseWeights,
                                                const bool dropIsolates);
    void writeCentralityBetweenness(const QString,
                                    const bool weights,
                                    const bool inverseWeights,
                                    const bool dropIsolates);
    void writeCentralityPower(const QString,
                              const bool weigths,
                              const bool inverseWeights,
                              const bool dropIsolates);
    void writeCentralityStress(const QString,
                               const bool weigths,
                               const bool inverseWeights,
                               const bool dropIsolates);
    void writeCentralityEccentricity(const QString,
                                     const bool weigths,
                                     const bool inverseWeights,
                                     const bool dropIsolates);
    void writeCentralityInformation(const QString,
                                    const bool weigths,
                                    const bool inverseWeights);
    void writePrestigeDegree(const QString, const bool weights,
                             const bool dropIsolates);
    void writePrestigeProximity(const QString, const bool weights,
                                const bool inverseWeights,
                                const bool dropIsolates);
    void writePrestigePageRank(const QString, const bool Isolates=false);


    void writeCliqueCensus(
            const QString fileName, const bool considerWeights
            );

    void writeClusteringCoefficient(const QString, const bool);

    void writeTriadCensus(const QString, const bool);


    /* DISTANCES, CENTRALITIES & PROMINENCE MEASURES */
    int distance(const int, const int,
                 const bool considerWeights, const bool inverseWeights);
    int diameter(const bool considerWeights, const bool inverseWeights);
    float averageGraphDistance(const bool considerWeights,
                               const bool inverseWeights, const bool dropIsolates);
    int connectedness();

    void createDistanceMatrix(const bool centralities=false,
                              const bool considerWeights=false,
                              const bool inverseWeights=true,
                              const bool dropIsolates=false);
    void centralityDegree(const bool weights, const bool dropIsolates=false);
    void centralityInformation(const bool considerWeights=false,
                               const bool inverseWeights=false);
    void centralityClosenessInfluenceRange(const bool considerWeights=false,
                                           const bool inverseWeights=false,
                                           const bool dropIsolates=false);

    void prestigeDegree(bool, bool);
    void prestigePageRank(const bool dropIsolates=false);
    void prestigeProximity(const bool considerWeights=false,
                           const bool inverseWeights=false,
                           const bool dropIsolates=false);

    /* REACHABILTY AND WALKS */
    int numberOfWalks(int v1, int v2,int length);
    void createNumberOfWalksMatrix(int length);
    void writeTotalNumberOfWalksMatrix(QString fn, QString netName, int length);
    void writeNumberOfWalksMatrix(QString fn, QString netName, int length);
    int reachable(int v1, int v2) ;
    QList<int> influenceRange(int v1);
    QList<int> influenceDomain(int v2);
    void reachabilityMatrix(const bool considerWeights=false,
                            const bool inverseWeights=false,
                            const bool dropIsolates=false);
    void writeReachabilityMatrix(QString fn, QString netName,
                                 const bool dropIsolates=false);


    float numberOfTriples(int v1);
    float countCliquesWith(int source, int size=0);

    bool addClique (const QList<int> &list);
    float countCliquesOfSize(int size );
    float localClusteringCoefficient(const long int &v1);
    float clusteringCoefficient ();

    bool triadCensus();
    void examine_MAN_label(int, int, int, Vertex*,  Vertex*, Vertex* );
    //	void eccentr_JordanCenter(); 				// TODO


    /* LAYOUTS */

    void layoutRandom( double maxWidth, double maxHeight );

    void layoutCircularRandom(double x0, double y0, double maxRadius);

    void layoutCircularByProminenceIndex(double x0, double y0, double maxRadius,
                                         int type, const bool considerWeights,
                                         const bool inverseWeights,
                                         const bool dropIsolates);

    void layoutLevelByProminenceIndex(double maxWidth, double maxHeight, int type,
                                      const bool considerWeights,
                                      const bool inverseWeights,
                                      const bool dropIsolates);

    void layoutVerticesSizeByProminenceIndex(int index,
                                             const bool considerWeights,
                                             const bool inverseWeights,
                                             const bool dropIsolates);

    void layoutForceDirectedSpringEmbedder(bool &dynamicMovement);

    void layoutForceDirectedFruchtermanReingold(bool dynamicMovement);


    /* CRAWLER */
    void terminateCrawlerThreads (QString reason);

    /**RANDOM NETWORKS*/



    void makeThingsLookRandom();


    void createRandomNetErdos (  const int &vert,
                                 const QString &model,
                                 const int &edges,
                                 const float &eprob,
                                 const QString &mode,
                                 const bool &diag);

    void createRandomNetRingLattice
    (int, int, double, double, double);

    void createSameDegreeRandomNetwork
    (int, int);

    void createRandomNetScaleFree (const int &n,
                                    const int &power,
                                    const int &m0,
                                    const int &m,
                                    const float &alpha,
                                    const QString &mode,
                                    const double &x0,
                                    const double &y0,
                                    const double &radius);


    void createRandomNetSmallWorld
    (int, int, double, double, double, double);

    int factorial (int);

    int relations();
    void addRelationFromGraph(QString relationName);

    /**  index stores the real position of each vertex inside m_graph.
     *  It starts at zero (0).
     *   We need to know the place of a vertex inside m_graph after adding
     *   or removing many vertices
     */
    H_Int index;

    // Stores the number of vertices at distance n from a given vertex
    H_Int sizeOfNthOrderNeighborhood;

    /* maps have O(logN) lookup complexity */
    /* Consider using tr1::hashmap which has O(1) lookup, but this is not ISO C++ yet :(   */


protected: 
    // Called from nodeMovement when a timerEvent occurs
    void timerEvent(QTimerEvent *event);


private:

    /**
     * List of pointers to the vertices.
     * A vertex stores all the info: links, colours, etc
    */
    Vertices m_graph;

    Parser *file_parser;	//file loader threaded class.

    WebCrawler_Parser *wc_parser;
    WebCrawler_Spider *wc_spider;

    /** private member functions */

    void addVertex (
            int v1, int val, int size, QString color,
            QString numColor, int numSize,
            QString label, QString labelColor, int labelSize,
            QPointF p, QString shape
            );

    void addEdge (int v1, int v2, float w, QString color, int reciprocal);

    /** methods used by createDistanceMatrix()  */
    void BFS(const int s, const bool computeCentralities,
             const bool dropIsolates);
    void dijkstra(const int s,const bool computeCentralities,
                  const bool inverseWeights, const bool dropIsolates);
    void minmax(
                float C, Vertex *v, float &max, float &min,
                int &maxNode, int &minNode
              ) ;
    void resolveClasses (float C, H_StrToInt &discreteClasses, int &classes);
    void resolveClasses (
                        float C, H_StrToInt &discreteClasses,
                        int &classes, int name
                        );


    QList<QString> m_relationsList;
    QList<int>  triadTypeFreqs; 	//stores triad type frequencies
    QList<int>  m_isolatedVerticesList;
    QHash <int, int> influenceRanges, influenceDomains;
    QHash <int, int> disconnectedVertices;
    QHash <int, int> unilaterallyConnectedVertices;

    H_StrToBool cliques_2_Vertex;
    H_StrToBool cliques_3_Vertex;
    H_StrToBool cliques_4_Vertex;

    Matrix  TM, DM, sumM, invAM, AM, invM;
    Matrix XM, XSM, XRM;
    stack<int> Stack;

    /** used in resolveClasses and createDistanceMatrix() */
    H_StrToInt discreteDPs, discreteDCs, discreteCCs, discreteBCs, discreteSCs;
    H_StrToInt discreteIRCCs, discreteECs, discreteEccentricities;
    H_StrToInt discretePCs, discreteICs,  discretePRPs, discretePPs;

    bool calculatedDP, calculatedDC, calculatedCentralities, dynamicMovement;
    bool calculatedPP, calculatedIRCC, calculatedIC, calculatedPRP;
    bool calculatedTriad;

    int m_precision, m_curRelation;
    float meanDC, varianceDC;
    float meanCC, varianceCC;
    float meanIRCC, varianceIRCC;
    float meanBC, varianceBC;
    float meanSC, varianceSC;
    float meanEC, varianceEC;
    float meanPC, variancePC;
    float meanIC, varianceIC;
    float meanDP, varianceDP;
    float meanPP, variancePP;
    float meanPRP, variancePRP;
    float minEccentricity, maxEccentricity, sumEccentricity;
    float minDP, maxDP, t_sumDP, sumDP, groupDP;
    float minDC, maxDC, t_sumDC, sumDC, groupDC;
    float minCC, maxCC, nomCC, denomCC, sumCC, groupCC, maxIndexCC;
    float minIRCC, maxIRCC, nomIRCC, denomIRCC, sumIRCC, groupIRCC;
    float minBC, maxBC, nomBC, denomBC, sumBC, groupBC, maxIndexBC;
    float minPC, maxPC, nomPC, denomPC, t_sumIC, sumPC, groupPC, maxIndexPC;
    float minSC, maxSC, nomSC, denomSC, sumSC, groupSC, maxIndexSC;
    float minEC, maxEC, nomEC, denomEC, sumEC, groupEC, maxIndexEC;
    float minIC, maxIC, nomIC, denomIC, sumIC, maxIndexIC;
    float minPRP, maxPRP, nomPRC, denomPRC, t_sumPC, t_sumPRP, sumPRP;
    float minPP, maxPP, nomPP, denomPP, sumPP, groupPP;
    float minCLC, maxCLC, averageCLC, d_factor;
    int maxNodeCLC, minNodeCLC;
    int classesDP, maxNodeDP, minNodeDP;
    int classesDC, maxNodeDC, minNodeDC;
    int classesCC, maxNodeCC, minNodeCC;
    int classesIRCC, maxNodeIRCC, minNodeIRCC;
    int classesBC, maxNodeBC, minNodeBC;
    int classesPC, maxNodePC, minNodePC;
    int classesSC, maxNodeSC, minNodeSC;
    int classesEC, maxNodeEC, minNodeEC;
    int classesEccentricity, maxNodeEccentricity, minNodeEccentricity;
    int classesIC, maxNodeIC, minNodeIC;
    int classesPRP, maxNodePRP, minNodePRP;
    int classesPP, maxNodePP, minNodePP;
    int sizeOfComponent;

    /** General & initialisation variables */

    long int m_totalVertices, graphDiameter, initVertexSize;
    int initVertexLabelSize, initVertexNumberSize;

    int isolatedVertices;
    float averGraphDistance, nonZeroDistancesCounter;
    int outboundEdgesVert, inboundEdgesVert, reciprocalEdgesVert;
    int timerId,  layoutType, canvasWidth, canvasHeight;

    bool order, initShowLabels, initNumbersInsideNodes;
    bool adjacencyMatrixCreated, symmetricAdjacencyMatrix, graphModified,
        distanceMatrixCreated;
    bool reachabilityMatrixCreated;
    bool m_undirected;

    QString VERSION, networkName, initEdgeColor, initVertexColor,
        initVertexNumberColor, initVertexLabelColor, initVertexShape;

    QDateTime actualDateTime;
};

#endif

