/***************************************************************************
 SocNetV: Social Networks Visualizer
 version: 1.5
 Written in Qt
 
                         graph.h  -  description
                             -------------------
    copyright            : (C) 2005-2014 by Dimitris B. Kalamaras
    email                : dimitris.kalamaras@gmail.com
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
#include <QList>
#include <QHash>
#include <QTextStream>
//FYI: stack is a wrapper around <deque> in C++, see: www.cplusplus.com/reference/stl/stack
#include <stack>
#include <map>

#include "vertex.h"
#include "matrix.h"
#include "parser.h"
#include "webcrawler.h"

using namespace std;

class QPointF;
class QDateTime;

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

typedef QHash <QString, float> H_StrToFloat;

class Graph:  public QObject{
    Q_OBJECT

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

    /** Slots to signals from GraphicsWidget and Parser*/
    void createEdge (int, int, float, QString, int, bool, bool);	//GW and Parser.
    void createEdge (int, int, float, int, bool, bool);		//GW
    void createEdge (int, int);					//WebCrawler
    void nodeMovement(bool state, int type, int cW, int cH);		//Called by MW to start movement

    void slotSetEdgeVisibility(int relation, int, int, bool);

    //auxiliary createVertex functions
    void createVertex(int i, QPointF p); 				//Called by GW
    void createVertex(int i, int canvasWidth, int canvasHeight); 	//Called by MW
    void createVertex(QString label, int i) ; 			//Called by WebCrawler

    /** Slots to signals from MainWindow */
    void changeRelation(int);
    void addRelationFromUser(QString relation);
    void setCanvasDimensions(int w, int h);
    void filterIsolateVertices ( bool );		//Called by MW to filter orphan vertices
    void filterEdgesByWeight (float, bool);		//Called by MW to filter edges over/under a weight
    void filterEdgesByRelation(int relation, bool status);

    void webCrawl( QString, int, int, bool);	//Called by MW to start a web crawler...

signals:
    /** Signals to MainWindow */
    void updateProgressDialog(int );
    void graphChanged();					//call to update MW widgets
    void selectedVertex(int);				//notifies MW who is the selected node

    void signalFileType (int, QString, int,int, bool);	//notifies MW what we have loaded.
    void statusMessage (QString message);			//updates statusbar message
    void addRelationToMW(QString newRelation);
    void describeDataset(QString);

    /** Signals to GraphicsWidget */
    void drawNode( int ,int,  QString, QString, int, QString, QString, int,
                   QPointF, QString, bool, bool, bool);	//call GW to draw a node

    void eraseNode (long int);						//erase node from GW
    void drawEdge(int, int, float, bool, bool, QString, bool);	//call GW to draw an edge
    void eraseEdge(int, int);					//emited from removeEdge() to GW to clear the edge item.
    void setEdgeVisibility (int, int, int, bool);			// emitted from each Vertex
    void setVertexVisibility(long int, bool);		//notifies GW to disable a node
    void setNodeSize(long int, int);
    void setNodeColor(long int, QString);
    void drawEdgeReciprocal(int, int);				//call GW to draw the edge as symmetric one
    void setLinkColor(long int, long int, QString);
    void addGuideCircle(int, int, int);				//call GW to draw a circular layout line somewhere.
    void addGuideHLine (int);					//call GW to draw a horizontal layout line somewhere.
    void moveNode(int, int, int);

    /** Signals to Vertice */
    void relationChanged(int);


public: 	
    /* INIT AND CLEAR*/
    Graph();
    void clear();
    ~Graph();			//destroy object

    void setSocNetV_Version (QString ver) { VERSION = ver; }

    void setShowLabels(bool toggle);
    void setShowNumbersInsideNodes(bool toggle);

    /*FILES (READ AND WRITE)*/
    bool loadGraph ( QString, bool,	int maxWidth, int maxHeight, int format, int two_sm_mode);

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

    void setInitVertexSize (long int);
    void setVertexSize(long int v, int );

    void setInitVertexShape (QString);
    void setVertexShape(int v, QString shape);
    QString shape(int v);

    void setInitVertexColor (QString color);
    void setVertexColor(long int v, QString color);
    void setAllVerticesColor(QString color);

    void setInitVertexNumberColor ( QString color);
    void setInitVertexNumberSize (int size);

    void setInitVertexLabelSize(int newSize);
    void setVertexLabelSize(int v, int newSize);

    void setInitVertexLabelColor(QString color);
    void setVertexLabel(int v, QString label);
    void setVertexLabelColor(int v1, QString color);
    QString label(int);

    void updateVertCoords(int v, int x, int y);

    int vertices() ;

    int outboundEdges (int i) ;
    int inboundEdges (int i) ;

    int outDegree(int);
    int inDegree(int);

    int verticesWithOutboundEdges();
    int verticesWithInboundEdges();
    int verticesWithReciprocalEdges();

    QList<int> verticesIsolated();

    /* EDGES */
    float hasEdge (int v1, int v2);
    void removeEdge (int v1, int v2);

    bool isWeighted();

    void setEdgeWeight (int v1, int v2, float w);
    void setInitEdgeColor(QString);

    void setEdgeColor(long int s, long int t, QString color);
    QString edgeColor (long int s, long int t);
    bool setAllEdgesColor(QString color);

    int totalEdges ();

    float density();

    bool symmetricEdge(int v1, int v2);
    bool isSymmetric();
    void symmetrize();

    void createAdjacencyMatrix(bool,bool);
    void invertAdjacencyMatrix();


    /* PRINT OUT TO FILES*/

    void writeDataSetToFile(const QString dir, const QString );
    void writeAdjacencyMatrixTo(QTextStream& os);
    void writeAdjacencyMatrix(const QString, const char*);

    void writeInvertAdjacencyMatrix(const QString filename,  const char*);
    void writeDistanceMatrix(const QString fn, const char*);
    void writeNumberOfGeodesicsMatrix(const QString fn, const char*);
    void writeEccentricity(const QString, const bool);

    friend QTextStream& operator <<  (QTextStream& os, Graph& m);

    void writeCentralityDegree(const QString, const bool);	//Writes the out-degree centralities to a file
    void writeCentralityCloseness(const QString, const bool);	//Writes the closeness centralities to a file
    void writeCentralityClosenessInfluenceRange(const QString, const bool);
    void writeCentralityBetweenness(const QString, const bool);	//Writes the betweenness centralities to a file
    void writeCentralityGraph(const QString, const bool);		//Writes the Graph centralities to a file
    void writeCentralityPower(const QString, const bool);		//Writes the Power centralities to a file
    void writeCentralityStress(const QString, const bool);		//Writes the Stress centralities to a file
    void writeCentralityEccentricity(const QString, const bool);	//Writes the Eccentr centralities to a file
    void writeCentralityInformation(const QString);			//Writes the Information centralities to a file

    void writePrestigeDegree(const QString, bool);
    void writePrestigeProximity(const QString, const bool);
    void writePrestigePageRank(const QString);


    void writeNumberOfCliques(
            const QString fileName, const bool considerWeights
            );

    void writeClusteringCoefficient(const QString, const bool);

    void writeTriadCensus(const QString, const bool);


    /* DISTANCES, CENTRALITIES & PROMINENCE MEASURES */
    int distance( int, int);
    int diameter();
    float averageGraphDistance();
    int connectedness();

    void createDistanceMatrix(bool);
    void centralityDegree(bool);
    void centralityInformation();
    void centralityClosenessInfluenceRange();

    void prestigeDegree(bool);
    int prestigePageRank();
    void prestigeProximity();

    /* REACHABILTY AND WALKS */
    int numberOfWalks(int v1, int v2,int length);
    void createNumberOfWalksMatrix(int length);
    void writeTotalNumberOfWalksMatrix(QString fn, QString netName, int length);
    void writeNumberOfWalksMatrix(QString fn, QString netName, int length);
    int reachable(int v1, int v2) ;
    QList<int> influenceRange(int v1);
    QList<int> influenceDomain(int v2);
    void reachabilityMatrix();
    void writeReachabilityMatrix(QString fn, QString netName);


    float numberOfTriples(int v1);
    float numberOfCliques(int v1);
    float numberOfCliques();
    float clusteringCoefficient(int v1);
    float clusteringCoefficient ();

    bool triadCensus();
    void examine_MAN_label(int, int, int, Vertex*,  Vertex*, Vertex* );
    //	void eccentr_JordanCenter(); 				// TODO


    /* LAYOUTS */

    void layoutRandom( double maxWidth, double maxHeight );

    void layoutCircularRandom(double x0, double y0, double maxRadius);

    void layoutCircularByProminenceIndex(
            double x0, double y0, double maxRadius, int type);

    void layoutLevelByProminenceIndex(
            double maxWidth, double maxHeight, int type);

    void layoutVerticesSizeByProminenceIndex(int index);

    void layoutForceDirectedSpringEmbedder(bool dynamicMovement);

    void layoutForceDirectedFruchtermanReingold(bool dynamicMovement);

    /**RANDOM NETWORKS*/
    void createRandomNetErdos
    (int, double);

    void createRandomNetRingLattice
    (int, int, double, double, double);

    void createSameDegreeRandomNetwork
    (int, int);

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

    Parser parser;	//file loader threaded class.

    WebCrawler crawler;

    /** private member functions */

    void addVertex (
            int v1, int val, int size, QString color,
            QString numColor, int numSize,
            QString label, QString labelColor, int labelSize,
            QPointF p, QString shape
            );

    void addEdge (int v1, int v2, float w, QString color, int reciprocal);

    /** methods used by createDistanceMatrix()  */
    void BFS(int, bool);	//Breadth-First Search function
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
    QHash <int, int> notStronglyConnectedVertices;
    H_StrToFloat edgesHash; // edges to weight hash. Key format "rel:source>target"

    Matrix  TM, DM, sumM, invAM, AM, invM;
    Matrix XM, XSM, XRM;
    stack<int> Stack;

    /** used in resolveClasses and createDistanceMatrix() */
    H_StrToInt discreteDPs, discreteDCs, discreteCCs, discreteBCs, discreteSCs;
    H_StrToInt discreteIRCCs, discreteECs, discreteEccentricities;
    H_StrToInt discretePCs, discreteICs,  discretePRCs, discretePPs;

    bool calculatedDP, calculatedDC, calculatedCentralities, dynamicMovement;
    bool calculatedPP, calculatedIRCC;

    int m_precision, m_curRelation;
    float meanDegree, varianceDegree;
    float meanCC, varianceCC;
    float meanIRCC, varianceIRCC;
    float meanPP, variancePP;
    float minEccentricity, maxEccentricity, sumEccentricity;
    float minDP, maxDP, sumDP, groupDP;
    float minDC, maxDC, sumDC, groupDC;
    float minCC, maxCC, nomCC, denomCC, sumCC, groupCC, maxIndexCC;
    float minIRCC, maxIRCC, nomIRCC, denomIRCC, sumIRCC, groupIRCC;
    float minBC, maxBC, nomBC, denomBC, sumBC, groupBC, maxIndexBC;
    float minPC, maxPC, sumPC, groupPC, maxIndexPC;
    float minSC, maxSC, nomSC, denomSC, sumSC, groupSC, maxIndexSC;
    float minEC, maxEC, nomEC, denomEC, sumEC, groupEC, maxIndexEC;
    float minIC, maxIC, nomIC, denomIC, sumIC, groupIC, maxIndexIC;
    float minPRC, maxPRC, nomPRC, denomPRC, sumPRC, groupPRC, maxIndexPRC;
    float minPP, maxPP, nomPP, denomPP, sumPP, groupPP;
    float minCLC, maxCLC, averageCLC, averageIC, averagePRC, dampingFactor;
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
    int classesPRC, maxNodePRC, minNodePRC;
    int classesPP, maxNodePP, minNodePP;
    int sizeOfComponent;

    /** General & initialisation variables */

    long int m_totalEdges, m_totalVertices, graphDiameter, initVertexSize;
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

