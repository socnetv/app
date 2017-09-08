/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 2.4
 Written in Qt
 
                         graph.h  -  description
                             -------------------
    copyright         : (C) 2005-2017 by Dimitris B. Kalamaras
    project site      : http://socnetv.org

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

#include "graphvertex.h"
#include "matrix.h"
#include "parser.h"
#include "webcrawler.h"

using namespace std;

static const int EDGE_DIRECTED                 = 0;
static const int EDGE_DIRECTED_OPPOSITE_EXISTS = 1;
static const int EDGE_RECIPROCAL_UNDIRECTED    = 2;

static const int FILE_GRAPHML           = 1;  // .GRAPHML .XML
static const int FILE_PAJEK             = 2;  // .PAJ .NET
static const int FILE_ADJACENCY         = 3;  // .ADJ .CSV .SM
static const int FILE_GRAPHVIZ          = 4;  // .DOT
static const int FILE_UCINET            = 5;  // .DL .DAT
static const int FILE_GML               = 6;  // .GML
static const int FILE_EDGELIST_WEIGHTED = 7;  // .CSV, .TXT, .LIST, LST, WLST
static const int FILE_EDGELIST_SIMPLE   = 8;  // .CSV, .TXT, .LIST, LST
static const int FILE_TWOMODE           = 9;  // .2SM .AFF
static const int FILE_UNRECOGNIZED      =-1;  // UNRECOGNIZED FILE FORMAT


static const int GRAPH_CHANGED_NONE                = 0;
static const int GRAPH_CHANGED_MINOR_OPTIONS       = 1;
static const int GRAPH_CHANGED_VERTICES_METADATA   = 2;
static const int GRAPH_CHANGED_EDGES_METADATA      = 3;
static const int GRAPH_CHANGED_POSITIONS           = 4;
static const int GRAPH_CHANGED_VERTICES            = 11;
static const int GRAPH_CHANGED_EDGES               = 12;
static const int GRAPH_CHANGED_VERTICES_AND_EDGES  = 13;
static const int GRAPH_CHANGED_NEW                 = 14;

static const int CLUSTERING_SINGLE_LINKAGE   = 0; //"single-link" or minimum
static const int CLUSTERING_COMPLETE_LINKAGE = 1; // "complete-link or maximum
static const int CLUSTERING_AVERAGE_LINKAGE  = 2;

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


class QPointF;



typedef QList<GraphVertex*> VList;
typedef QHash <QString, int> H_StrToInt;
typedef QHash <long int, long int> H_Int;
typedef QHash <float, long int> H_f_i;
typedef QPair <float, bool> pair_f_b;
typedef QPair <int, pair_f_b > pair_i_fb;
typedef QHash < int, pair_i_fb > H_edges;
typedef QHash<QString, bool> H_StrToBool;
typedef QList<int> L_int;
typedef QVector<int> V_int;
typedef QVector<QString> V_str;



struct ClickedEdge {
    int v1;
    int v2;
};



typedef QPair<int, int> SelectedEdge;



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


// implement a min-priority queue
class GraphDistancesCompare {
    public:
    bool operator()(GraphDistance& t1, GraphDistance& t2)
    {
       if (t1.distance == t2.distance)
            return t1.target > t2.target;
       return t1.distance > t2.distance;  //minimum priority
       // Returns true if t1 is closer than t2
       // else
    }
};


/**
  TODO & KNOWN BUGS:
    \todo Group edge editing: i.e. change weight or color.
    \todo Enrich Node properties dialog
    \todo Update app icons
    \todo - CHECK weighted networks results (IRCC and distance matrix with other combinations)
    \todo - CHECK graphWeighted corner case results, when !graphModified.
    \todo - CHECK graphConnectedness() algorithm implementation (m_vertexPairsUnilaterallyConnected)

  \bug Create d-regular, undirected, ask for closeness, it says we are on a disconnected graph
  \bug Crash on Graphml files with textlabels instead of nodenumbers (i.e. nets/killer.graphml)
  \bug wontfix Crash on Graphml files with html special chars in node/edge labels
  \bug Pajek files with no ic / label in nodes are displayed without colors???
  \bug wrong default edge colors (not the ones used by Settings) after loading GraphML files.
  \bug Resizing the MW view does not resize/reposition the layout guides
  \bug Fruchterman-Reingold model fixes some nodes to (1,1) breaking the layout


  TODO
  Change Menus to:
  Matrices
  Cohesion/Connectedness: Density, Reachability, and All distance and Walks, Connectivity, Reciprocity, Transitivity ?, Clu Cof
  Prominence: Centrality and Prestige
  Subgroups / Communities: Cliques (later clans etc), (later path distance MDS) Components, Blocks and Cutpoints,
  Structural Equivalence: HCA, Similarity (later MDS), Block modelling, CONCOR

  */


/**
 * @brief The Graph class
 * This is the main class for a Graph, used in conjuction with GraphVertex, Parser and Matrix objects.
 *   Graph class methods are the interface to various analysis algorithms
 *   GraphVertex class holds each vertex data (colors, strings, statistics, etc)
 *   Matrix class holds the adjacency matrix of the network.
 *   Parser class loads files of networks.
 */
class Graph:  public QObject{
    Q_OBJECT
    QThread file_parserThread;
    QThread wc_parserThread;
    QThread wc_spiderThread;

public slots:

    int relationCurrent();
    QString relationCurrentName() const;
    void relationCurrentRename(const QString &newName, const bool &notifyMW=false);

    /** Slots to signals from Parser */
    void vertexCreate(const int &num, const int &size, const QString &nodeColor,
                       const QString &numColor, const int &numSize,
                       const QString &label, const QString &lColor,
                       const int &labelSize, const QPointF &p, const QString &nodeShape,
                       const bool &signalMW
                        );//Main vertex creation call

    void graphFileLoaded(const int &fileType,
                         const QString &fName=QString::null,
                         const QString &netName=QString::null,
                         const int &totalNodes=0,
                         const int &totalLinks=0,
                         const bool &undirected=false,
                         const QString &message=QString::null);

    void vertexRemoveDummyNode(int);
    void graphLoadedTerminateParserThreads (QString reason);

    /** Slots to signals from GraphicsWidget and Parser*/
    void edgeCreate  (const int &v1, const int &v2, const float &weight,
                      const QString &color ,
                      const int &type=0,
                      const bool &drawArrows=true, const bool &bezier=false,
                      const QString &label=QString::null,
                      const bool &signalMW=true);
    void edgeCreateWebCrawler (const int &source, const int &target);

    void edgeVisibilitySet(int relation, int, int, bool);

    //auxiliary vertexCreate functions
    void vertexCreateAtPos(const QPointF &p);
    void vertexCreateAtPosRandom(const bool &signalMW=false);
    void vertexCreateAtPosRandomWithLabel(const int &i,
                                          const QString &label,
                                          const bool &signalMW=false) ;
    /** Slots to signals from MainWindow */

    void relationSet(int relNum=RAND_MAX, const bool notifyMW=true);
    void relationNext();
    void relationPrev();
    void canvasSizeSet(const int w, const int h);
    double canvasMaxRadius() const;
    float canvasMinDimension() const;
    double canvasVisibleX(const double &x) const ;
    double canvasVisibleY(const double &y) const ;
    double canvasRandomX()  const;
    double canvasRandomY()  const;
    void vertexIsolatedAllToggle ( const bool &toggle);
    void vertexClickedSet(const int &v);
    void edgeClickedSet(const int &v1, const int &v2) ;
    void edgeFilterByWeight (float, bool);		//Called by MW to filter edges over/under a weight
    void edgeFilterByRelation(int relation, bool status);
    void edgeFilterUnilateral(const bool &toggle);
    void webCrawl(QString, int, int, bool extLinks, bool intLinks);	//Called by MW to start a web crawler...

    QString htmlEscaped (QString str) const;

signals:
    /** Signals to MainWindow */
    void signalProgressBoxCreate(const int max=0, const QString msg="Please wait");
    void signalProgressBoxKill(const int max=0);
    void signalProgressBoxUpdate(const int &count=0 );
    void signalGraphModified(const int &graphStatus,
                      const bool &undirected,
                      const int &vertices,
                      const int &edges,
                      const float &density);

    void signalGraphLoaded (const int &fileType,
                            const QString &fileName=QString::null,
                            const QString &netName=QString::null,
                            const int &totalNodes=0,
                            const int &totalLinks=0,
                            const QString &message=QString::null
            );
    void signalGraphSaved(const int &status);

    void statusMessage (const QString &message);
    void signalDatasetDescription(QString);

    void signalNodeClickedInfo(const int &number=0,
                                    const QPointF &p=QPointF(),
                                    const QString &label=QString::null,
                                    const int &inDegree=0,
                                    const int &outDegree=0,
                                    const float &clc=0);
    void signalEdgeClickedInfo (const int &v1=0,
                                const int &v2=0,
                                const float &weight=0,
                                const bool &undirected=false);
    void signalRelationAddToMW(const QString &newRelation, const bool &changeRelation=true);
    void signalRelationsClear();
    void signalRelationRenamedToMW(const QString newRelName);
    void signalRelationChangedToGW(int);
    void signalRelationChangedToMW(const int &relIndex=RAND_MAX);
    void signalSelectionChanged(const int &selectedVertices,
                                const int &selectedEdges);

    /** Signals to GraphicsWidget */
    void drawNode( const int &num, const int &size, const QString &nodeShape,
                   const QString &nodeColor,
                   const QString &numberColor, const int &numSize,
                   const int &numDistance,
                   const QString &label,
                   const QString &labelColor, const int &labelSize,
                   const int &labelDistance,
                   const QPointF &p
                    );

    void eraseNode (long int);						//erase node from GW
    //call GW to draw an edge
    void drawEdge ( const int &v1, const int &v2, const float &weight,
                    const QString &label="",
                    const QString &color="black",
                    const int &type=0, const bool arrows=true,
                    const bool &bezier=false,  const bool &weightNumbers=false);
    void eraseEdge(const long int &, const long int &);					//emited from edgeRemove() to GW to clear the edge item.
    void setEdgeVisibility (int, int, int, bool);			// emitted from each GraphVertex
    void setVertexVisibility(long int, bool);		//notifies GW to disable a node
    void setNodePos(const int &, const qreal &, const qreal &);
    void setNodeSize(const long int &v, const int &size);
    void setNodeShape(const long int v, const QString &shape);
    void setNodeColor(const long int v, const QString &color);
    void setNodeLabel(long int, QString);
    void setNodeNumberSize(const long int &, const int &);
    void setNodeNumberDistance(const long int &, const int &);
    void setNodeLabelSize(const long int &, const int &);
    void setNodeLabelColor(const long int &, const QString &color);
    void setNodeLabelDistance(const long int &, const int &);

    void setEdgeWeight (const long int &v1, const long int &v2, const float &weight);
    void setEdgeUndirected(const long int &v1, const long int &v2, const float &weight);
    void setEdgeColor(const long int &v1,
                         const long int &v2,
                         const QString &color);
    void setEdgeLabel (const long int &v1,
                       const long int &v2,
                       const QString &label);
    void addGuideCircle(const double&, const double&, const double&);
    void addGuideHLine (const double&y0);




    /** Signals to Crawler threads */
    void  operateSpider();


public: 	
    /* INIT AND CLEAR*/
    Graph();
    void clear(const bool &exit=false);
    ~Graph();			//destroy object

    void setSocNetV_Version (QString ver) { VERSION = ver; }


    /*FILES (READ AND WRITE)*/
    QString graphName() const;
    void graphLoad (const QString m_fileName,
                    const QString m_codecName,
                    const int format,
                    const int two_sm_mode,
                    const QString delimiter=QString::null);

    void graphSave(const QString &fileName,
                   const int &fileType,
                   const bool &saveEdgeWeights=true);
    bool graphSaveToPajekFormat (const QString &fileName,
                                 QString networkName="",
                                 int maxWidth=0, int maxHeight=0);
    bool graphSaveToAdjacencyFormat (const QString &fileName,
                                     const bool &saveEdgeWeights=true);

    bool graphSaveToGraphMLFormat (const QString &fileName,
                                   QString networkName="",
                                   int maxWidth=0, int maxHeight=0);
    bool graphSaveToDotFormat (QString fileName);
    int graphFileFormat() const;
    bool graphFileFormatExportSupported(const int &fileFormat) const;

    QString graphMatrixTypeToString(const int &matrixType) const;
    int graphMatrixStrToType(const QString &matrix) const;

    QString graphMetricTypeToString(const int &metricType) const;
    int graphMetricStrToType(const QString &metricStr) const;

    QString graphClusteringMethodTypeToString(const int &methodType) const;
    int graphClusteringMethodStrToType(const QString &method) const;

    /* RELATIONS */
    int relations();
    void relationsClear();
    void relationAdd(const QString &relName, const bool &changeRelation=false);

    /* VERTICES */
    int vertexNumberMax();
    int vertexNumberMin();

    int vertexDegreeOut(int);
    int vertexDegreeIn(int);
    QList<int> vertexNeighborhoodList(const int &v1);

    bool vertexIsolated(const long int &v1) const;

    int vertexExists(const long int &v1 );
    int vertexExists(const QString &label);
    void vertexRemove (long int );

    void vertexSizeInit (const long int);
    void vertexSizeSet(const long int &v, const int &newsize );
    void vertexSizeAllSet(const int newsize);
    int vertexSize(const long int &v);

    void vertexShapeInit (const QString);
    void vertexShapeSet(const int v, const QString shape);
    void vertexShapeAllSet(const QString shape);
    QString vertexShape(const int &v);

    void vertexColorInit (const QString &color);
    void vertexColorSet(const long &v, const QString &color);
    void vertexColorAllSet(const QString &color);
    QColor vertexColor(const long int &v);

    void vertexNumberColorInit ( QString color);
    void vertexNumberSizeInit (const int &size);
    void vertexNumberSizeSet(const long int &v, const int &newsize );
    void vertexNumberSizeSetAll (const int &size);
    void vertexNumberDistanceInit (const int &distance);
    void vertexNumberDistanceSet(const long int &v, const int &newDistance );
    void vertexNumberDistanceSetAll (const int &newDistance);


    void vertexLabelsVisibilitySet(bool toggle);
    void vertexLabelSizeInit(int newSize);
    void vertexLabelSizeSet(const long int &v, const int &newsize );
    void vertexLabelSizeAllSet (const int &);
    void vertexLabelColorInit(QString color);
    void vertexLabelSet(int v, QString label);
    void vertexLabelColorSet(int v1, QString color);
    void vertexLabelColorAllSet(const QString &color);
    QString vertexLabel(const long int &v1);
    void vertexLabelDistanceInit (const int &distance);
    void vertexLabelDistanceSet(const long int &v, const int &newDistance );
    void vertexLabelDistanceAllSet (const int &newDistance);



    void vertexPosSet(const int &v, const int &x, const int &y);
    QPointF vertexPos(const int &v1);
    int vertexClicked() const;

    int vertices(const bool &dropIsolates=false, const bool &countAll=false, const bool &recount=false) ;

    int vertexEdgesOutbound (int i) ;
    int vertexEdgesInbound (int i) ;

    int verticesWithOutboundEdges();
    int verticesWithInboundEdges();
    int verticesWithReciprocalEdges();

    QList<int> verticesListIsolated();
    QList<int> verticesList();
    QSet<int> verticesSet();



    void verticesCreateSubgraph(QList<int> vList,
                                const int &type = SUBGRAPH_CLIQUE,
                                const int &center = 0);


    qreal graphDistanceEuclidean(const QPointF &a, const QPointF &b);
    qreal graphDistanceEuclidean(const QPointF &a);
    int sign(const qreal &D);

    qreal layoutForceDirected_F_rep(const QString model, const qreal &dist,
                                    const qreal &optimalDistance) ;
    qreal layoutForceDirected_F_att(const QString model, const qreal &dist,
                                    const qreal &optimalDistance) ;

    void layoutForceDirected_Eades_moveNodes(const qreal &c4);
    void layoutForceDirected_FR_moveNodes(const qreal &temperature) ;

    qreal layoutForceDirected_FR_temperature(const int iteration) const;
    qreal computeOptimalDistance(const int &V);
    void compute_angles( const QPointF &Delta,
                         const qreal &dist,
                         qreal &angle1,
                         qreal &angle2,
                         qreal &degrees1,
                         qreal &degrees2 );

    /* EDGES */
    int edgesEnabled();
    ClickedEdge edgeClicked();
    float edgeExists(const long &v1, const long &v2, const bool &undirected=false);

    void edgeRemove (const long int &v1, const long int &v2,
                     const bool &removeOpposite=false);
    bool edgeSymmetric(const long &v1, const long &v2);
    void edgeUndirectedSet(const long int &v1, const long int &v2, const float &w);

    void edgeWeightSet (const long int &v1, const long int &v2,
                        const float &w,
                        const bool &undirected=false);
    float edgeWeight(const long int &v1, const long int &v2) const;
    void edgeWeightNumbersVisibilitySet (const bool &toggle);

    void edgeLabelSet(const long int &v1, const long int &v2, const QString &label);
    QString edgeLabel (const long int &v1, const long int &v2) const;
    void edgeLabelsVisibilitySet (const bool &toggle);

    void edgeColorInit(const QString &);
    void edgeColorSet(const long int &v1, const long int &v2, const QString &color);
    QString edgeColor (const long int &v1, const long int &v2);
    bool edgeColorAllSet(const QString &color, const int &threshold=RAND_MAX);

    //GRAPH methods
    void graphModifiedSet(const int &graphNewStatus, const bool&signalMW=true);
    bool graphModified() const ;
    bool graphSaved() const;
    bool graphLoaded() const;

    void graphSelectionChanged(const QList<int> &selectedVertices,
                               const QList<SelectedEdge> &selectedEdges);

    QList<int> graphSelectedVertices() const;
    int graphSelectedVerticesCount() const;
    int graphSelectedVerticesMin() const;
    int graphSelectedVerticesMax() const;

    QList<SelectedEdge> graphSelectedEdges() const;
    int graphSelectedEdgesCount() const;

    int graphGeodesics();

    float graphDensity();
    bool graphWeighted();

    float graphReciprocity();

    bool graphSymmetric();
    void graphSymmetrize();
    void graphSymmetrizeStrongTies(const bool &allRelations=false);

    void graphCocitation();

    void graphUndirectedSet(const bool &toggle, const bool &signalMW=true);
    bool graphUndirected();

    void graphMatrixAdjacencyCreate(const bool dropIsolates=false,
                               const bool considerWeights=true,
                               const bool inverseWeights=false,
                               const bool symmetrize=false );

    bool graphMatrixAdjacencyInvert(const QString &method="lu");


    void graphMatrixSimilarityMatchingCreate(Matrix &AM,
                                             Matrix &SEM,
                                             const int &measure=METRIC_SIMPLE_MATCHING,
                                             const QString &varLocation="Rows",
                                             const bool &diagonal=false,
                                             const bool &considerWeights=true);

    void graphMatrixSimilarityPearsonCreate (Matrix &AM,
                                             Matrix &PCC,
                                             const QString &varLocation="Rows",
                                             const bool &diagonal=false);

    void graphMatrixDissimilaritiesCreate(Matrix &INPUT_MATRIX,
                                          Matrix &DSM,
                                          const int &metric,
                                          const QString &varLocation,
                                          const bool &diagonal,
                                          const bool &considerWeights);

    /* REPORT EXPORTS */

    void writeDataSetToFile(const QString dir, const QString );

    void writeMatrixAdjacencyTo(QTextStream& os,
                                const bool &saveEdgeWeights=true);

    void writeReciprocity( const QString fileName, const bool considerWeights=false);

    void writeMatrix(const QString &fileName,
                     const int &matrix=MATRIX_ADJACENCY,
                     const bool &considerWeights=true,
                     const bool &inverseWeights=false,
                     const bool &dropIsolates=false,
                     const QString &varLocation="Rows",
                     const bool &simpler=false);

    void writeMatrixHTMLTable(QTextStream &outText, Matrix &M,
                              const bool &markDiag=true,
                              const bool &plain=false,
                              const bool &printInfinity=true,
                              const bool &dropIsolates=false);

    void writeMatrixAdjacency(const QString fileName,
                              const bool &markDiag=true);

    void writeMatrixAdjacencyPlot(const QString fileName,
                                      const bool &simpler=false);

    void writeMatrixAdjacencyInvert(const QString &filename,
                                    const QString &method);

    void writeMatrixLaplacianPlainText(const QString &filename);
    void writeMatrixDegreeText(const QString &filename);

    void writeMatrixDistancesPlainText(const QString &fn,
                                       const bool &considerWeights,
                                       const bool &inverseWeights,
                                       const bool &dropIsolates);

    void writeMatrixShortestPathsPlainText(const QString &fn,
                                               const bool &considerWeights,
                                               const bool &inverseWeights);

    void writeMatrixDissimilarities(const QString fileName,
                                    const QString &metricStr,
                                    const QString &varLocation,
                                    const bool &diagonal,
                                    const bool &considerWeights) ;

    void writeMatrixSimilarityMatchingPlain(const QString fileName,
                                            const int &measure=METRIC_SIMPLE_MATCHING,
                                            const QString &matrix = "adjacency",
                                            const QString &varLocation="rows",
                                            const bool &diagonal=false,
                                            const bool &considerWeights=true);

    void writeMatrixSimilarityMatching(const QString fileName,
                                       const QString &measure="Simple",
                                       const QString &matrix = "adjacency",
                                       const QString &varLocation="rows",
                                       const bool &diagonal=false,
                                       const bool &considerWeights=true);


    void writeMatrixSimilarityPearson(const QString fileName,
                                      const bool considerWeights,
                                      const QString &matrix = "adjacency",
                                      const QString &varLocation="rows",
                                      const bool &diagonal=false);

    void writeMatrixSimilarityPearsonPlainText(const QString fileName,
                                               const bool considerWeights,
                                               const QString &matrix = "adjacency",
                                               const QString &varLocation="rows",
                                               const bool &diagonal=false);

    void writeEccentricity( const QString fileName, const bool considerWeights=false,
                            const bool inverseWeights=false, const bool dropIsolates=false);

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
    void writeCentralityEigenvector(const QString,
                                    const bool &weigths=true,
                                    const bool &inverseWeights = false,
                                    const bool &dropIsolates=false);
    void writePrestigeDegree(const QString, const bool weights,
                             const bool dropIsolates);
    void writePrestigeProximity(const QString, const bool weights,
                                const bool inverseWeights,
                                const bool dropIsolates);
    void writePrestigePageRank(const QString, const bool Isolates=false);


    void writeClusteringHierarchical(const QString &fileName,
                                     const QString &matrix = "Adjancency",
                                     const QString &metric = "Manhattan",
                                     const QString &method = "Complete",
                                     const bool &diagonal = false,
                                     const bool &dendrogram = false,
                                     const bool &considerWeights=true,
                                     const bool &inverseWeights=false,
                                     const bool &dropIsolates=false);

    void writeClusteringHierarchicalResultsToStream(QTextStream& outText,
                                                    const int N,
                                               const bool &dendrogram = false);

    void writeCliqueCensus(
            const QString fileName, const bool considerWeights
            );

    void writeClusteringCoefficient(const QString, const bool);

    void writeTriadCensus(const QString, const bool);






    /* DISTANCES, CENTRALITIES & PROMINENCE MEASURES */
    int graphConnectedness(const bool updateProgress=false) ;

    bool graphReachable(const int &v1, const int &v2) ;
    void graphMatrixReachabilityCreate() ;

    int graphDiameter(const bool considerWeights, const bool inverseWeights);

    int graphDistanceGeodesic(const int v1, const int v2,
                              const bool considerWeights,
                              const bool inverseWeights);

    float graphDistanceGeodesicAverage(const bool considerWeights,
                               const bool inverseWeights, const bool dropIsolates);

    void graphDistanceGeodesicCompute(const bool &computeCentralities=false,
                              const bool &considerWeights=false,
                              const bool &inverseWeights=true,
                              const bool &dropIsolates=false);

    void graphMatrixDistanceGeodesicCreate(const bool &considerWeights=false,
                                     const bool &inverseWeights=true,
                                     const bool &dropIsolates=false);

    void graphMatrixShortestPathsCreate(const bool &considerWeights=false,
                                     const bool &inverseWeights=true,
                                     const bool &dropIsolates=false) ;

    void centralityDegree(const bool &weights=true,
                          const bool &dropIsolates=false);
    void centralityInformation(const bool considerWeights=false,
                               const bool inverseWeights=false);
    void centralityEigenvector(const bool &considerWeights=false,
                               const bool &inverseWeights=false,
                               const bool &dropIsolates=false);
    void centralityClosenessIR(const bool considerWeights=false,
                                           const bool inverseWeights=false,
                                           const bool dropIsolates=false);

    void prestigeDegree(const bool &weights, const bool &dropIsolates=false);
    void prestigePageRank(const bool &dropIsolates=false);
    void prestigeProximity(const bool considerWeights=false,
                           const bool inverseWeights=false,
                           const bool dropIsolates=false);

    /* REACHABILTY AND WALKS */
    int walksBetween(int v1, int v2,int length);
    void graphWalksMatrixCreate(const int &N=0, const int &length=0,
                                   const bool &updateProgress=false);
    void writeWalksTotalMatrixPlainText(const QString &fn);
    void writeWalksOfLengthMatrixPlainText(const QString &fn, const int &length);
    void writeMatrixWalks (const QString &fn,
                             const int &length=0,
                             const bool &simpler=false);

    QList<int> vertexinfluenceRange(int v1);
    QList<int> vertexinfluenceDomain(int v2);

    void writeReachabilityMatrixPlainText(const QString &fn, const bool &dropIsolates=false);


    float numberOfTriples(int v1);

    /* CLIQUES, CLUSTERING, TRIADS */
    void graphCliques(QSet<int> R=QSet<int>(), QSet<int> P=QSet<int>(), QSet<int> X=QSet<int>() );
    void graphCliqueAdd (const QList<int> &clique);
    int graphCliquesContaining(const int &actor, const int &size=0);
    int graphCliquesOfSize(const int &size );

    void graphClusteringHierarchical(Matrix &STR_EQUIV,
                                     const int &metric,
                                     const int &method,
                                     const bool &diagonal=false,
                                     const bool &diagram=false,
                                     const bool &considerWeights=true,
                                     const bool &inverseWeights=false,
                                     const bool &dropIsolates=false);
    float clusteringCoefficientLocal(const long int &v1);
    float clusteringCoefficient (const bool updateProgress=false);

    bool graphTriadCensus();
    void triadType_examine_MAN_label(int, int, int, GraphVertex*,  GraphVertex*, GraphVertex* );
    //	void eccentr_JordanCenter(); 				// TODO



    /* LAYOUTS */

    void layoutRandom();

    void layoutRadialRandom(const bool &guides=true);

    void layoutCircular(const double &x0,
                        const double &y0,
                        const double &newRadius,
                        const bool &guides=false);


    void layoutByProminenceIndex ( int prominenceIndex, int layoutType,
                                   const bool considerWeights=false,
                                   const bool inverseWeights=false,
                                   const bool dropIsolates=false);


    void layoutVertexSizeByIndegree();
    void layoutVertexSizeByOutdegree();


    void layoutForceDirectedSpringEmbedder(const int maxIterations);

    void layoutForceDirectedFruchtermanReingold(const int maxIterations);

    void layoutForceDirectedKamadaKawai(const int maxIterations=500,
                                        const bool considerWeights=false,
                                        const bool inverseWeights=false,
                                        const bool dropIsolates=false,
                                        const QString &initialPositions="current");

    /* CRAWLER */
    void webCrawlTerminateThreads (QString reason);

    /**RANDOM NETWORKS*/
    void randomizeThings();

    void randomNetErdosCreate (  const int &N,
                                 const QString &model,
                                 const int &m,
                                 const float &p,
                                 const QString &mode,
                                 const bool &diag);

    void randomNetRingLatticeCreate (const int &N, const int &degree,
                                      const bool updateProgress=false);

    void randomNetRegularCreate (const int &N,
                                    const int &degree,
                                    const QString &mode,
                                    const bool &diag);

    void randomNetScaleFreeCreate (const int &N,
                                    const int &power,
                                    const int &m0,
                                    const int &m,
                                    const float &alpha,
                                    const QString &mode);

    void randomNetSmallWorldCreate(const int &N, const int &degree,
                                   const double &beta, const QString &mode);

    int factorial (int);


    /**  vpos stores the real position of each vertex inside m_graph.
     *  It starts at zero (0).
     *   We need to know the place of a vertex inside m_graph after adding
     *   or removing many vertices
     */
    H_Int vpos;

    // Stores the number of vertices at distance n from a given vertex
    H_f_i sizeOfNthOrderNeighborhood;

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
    VList m_graph;

    Parser *file_parser;	//file loader threaded class.

    WebCrawler_Parser *wc_parser;
    WebCrawler_Spider *wc_spider;

    /** private member functions */
    void vertexAdd  ( const int &v1, const int &val, const int &size,
                      const QString &color, const QString &numColor,
                      const int &numSize, const QString &label,
                      const QString &labelColor, const int &labelSize,
                      const QPointF &p, const QString &shape );

    void edgeAdd (const int &v1, const int &v2, const float &weight,
                  const int &type,
                  const QString &label,
                  const QString &color
                  );

    /** methods used by graphDistanceGeodesicCompute()  */
    void BFS(const int &s, const int &si, const bool &computeCentralities=false,
             const bool &dropIsolates=false);
    void dijkstra(const int &s, const int &si, const bool &computeCentralities=false,
                  const bool &inverseWeights=false,
                  const bool &dropIsolates=false);

    void minmax(
                float C, GraphVertex *v, float &max, float &min,
                int &maxNode, int &minNode
              ) ;
    void resolveClasses (float C, H_StrToInt &discreteClasses, int &classes);
    void resolveClasses (
                        float C, H_StrToInt &discreteClasses,
                        int &classes, int name
                        );


    QList<QString> m_relationsList;
    QList<int>  triadTypeFreqs; 	//stores triad type frequencies
    QList<int>  m_isolatedVerticesList,m_verticesList, m_graphFileFormatExportSupported;
    QSet<int> m_verticesSet;
    QList<SelectedEdge> m_selectedEdges;
    QList<int> m_selectedVertices;
    QHash <int, int> influenceRanges, influenceDomains;
    QHash <int, int> m_vertexPairsNotConnected;
    QHash <int, int> m_vertexPairsUnilaterallyConnected;

    QMap <int, L_int > m_cliques;

    QList <float> m_clusteringLevel;
    QMap <int, V_int> m_clustersPerSequence;


    QMap<QString, V_int> m_clustersByName;
    QMap<int, V_str> m_clusterPairNamesPerSeq;

    Matrix  SIGMA, DM, sumM, invAM, AM, invM, WM;
    Matrix XM, XSM, XRM, CLQM;

    stack<int> Stack;

    /** used in resolveClasses and graphDistanceGeodesicCompute() */
    H_StrToInt discreteDPs, discreteSDCs, discreteCCs, discreteBCs, discreteSCs;
    H_StrToInt discreteIRCCs, discreteECs, discreteEccentricities;
    H_StrToInt discretePCs, discreteICs,  discretePRPs, discretePPs, discreteEVCs;

    int m_precision, m_fieldWidth, m_curRelation, m_fileFormat, m_vertexClicked;
    ClickedEdge m_clickedEdge;
    float edgeWeightTemp, edgeReverseWeightTemp;
    float meanSDC, varianceSDC;
    float meanSCC, varianceSCC;
    float meanIRCC, varianceIRCC;
    float meanSBC, varianceSBC;
    float meanSSC, varianceSSC;
    float meanEC, varianceEC;
    float meanSPC, varianceSPC;
    float meanIC, varianceIC;
    float meanEVC, varianceEVC;
    float meanSDP, varianceSDP;
    float meanPP, variancePP;
    float meanPRP, variancePRP;
    float minEccentricity, maxEccentricity, sumEccentricity;
    float minSDP, maxSDP, sumDP, sumSDP, groupDP;
    float minSDC, maxSDC, sumDC, sumSDC, groupDC;
    float minSCC, maxSCC, nomSCC, denomSCC, sumCC, sumSCC, groupCC, maxIndexCC;
    float minIRCC, maxIRCC, nomIRCC, denomIRCC, sumIRCC, groupIRCC;
    float minSBC, maxSBC, nomSBC, denomSBC, sumBC, sumSBC, groupSBC, maxIndexBC;
    float minSPC, maxSPC, nomSPC, denomSPC, t_sumIC, sumSPC, groupSPC, maxIndexPC;
    float minSSC, maxSSC, sumSC, sumSSC, groupSC, maxIndexSC;
    float minEC, maxEC, nomEC, denomEC, sumEC, groupEC, maxIndexEC;
    float minIC, maxIC, nomIC, denomIC, sumIC, maxIndexIC;
    float minEVC, maxEVC, nomEVC, denomEVC, sumEVC, sumSEVC, groupEVC;
    float minPRP, maxPRP, nomPRC, denomPRC, sumPC, t_sumPRP, sumPRP;
    float minPP, maxPP, nomPP, denomPP, sumPP, groupPP;

    float minCLC, maxCLC, averageCLC,varianceCLC, d_factor;
    int maxNodeCLC, minNodeCLC;
    int classesSDP, maxNodeDP, minNodeDP;
    int classesSDC, maxNodeSDC, minNodeSDC;
    int classesSCC, maxNodeSCC, minNodeSCC;
    int classesIRCC, maxNodeIRCC, minNodeIRCC;
    int classesSBC, maxNodeSBC, minNodeSBC;
    int classesSPC, maxNodeSPC, minNodeSPC;
    int classesSSC, maxNodeSSC, minNodeSSC;
    int classesEC, maxNodeEC, minNodeEC;
    int classesEccentricity, maxNodeEccentricity, minNodeEccentricity;
    int classesIC, maxNodeIC, minNodeIC;
    int classesPRP, maxNodePRP, minNodePRP;
    int classesPP, maxNodePP, minNodePP;
    int classesEVC, maxNodeEVC, minNodeEVC;
    float sizeOfComponent;

    /** General & initialisation variables */

    int graphModifiedFlag;
    long int m_totalVertices, m_totalEdges, m_graphDiameter, initVertexSize;
    int initVertexLabelSize, initVertexNumberSize;
    int initVertexNumberDistance, initVertexLabelDistance;
    bool order;
    bool initEdgeWeightNumbers, initEdgeLabels;
    float m_graphAverageDistance, m_graphGeodesicsCount;
    float m_graphDensity;
    float m_graphReciprocityArc, m_graphReciprocityDyad;
    int m_graphReciprocityTiesReciprocated;
    int m_graphReciprocityTiesNonSymmetric;
    int m_graphReciprocityTiesTotal;
    int m_graphReciprocityPairsReciprocated;
    int m_graphReciprocityPairsTotal;

    int m_graphConnectedness;
    int outboundEdgesVert, inboundEdgesVert, reciprocalEdgesVert;
    int timerId,  canvasWidth, canvasHeight;
    bool calculatedEdges;
    bool calculatedVertices, calculatedVerticesList, calculatedVerticesSet;
    bool calculatedAdjacencyMatrix, calculatedDistances, calculatedCentralities;
    bool calculatedIsolates;
    bool calculatedEVC;
    bool calculatedDP, calculatedDC, calculatedPP;
    bool calculatedIRCC, calculatedIC, calculatedPRP;
    bool calculatedTriad;
    bool calculatedGraphSymmetry, calculatedGraphReciprocity;
    bool calculatedGraphDensity, calculatedGraphWeighted;
    bool calculatedGraphConnectedness;
    bool m_undirected, m_symmetric, m_isWeighted;

    QString VERSION, fileName, m_graphName, initEdgeColor, initVertexColor,
        initVertexNumberColor, initVertexLabelColor, initVertexShape;
    QString htmlHead, htmlHeadLight, htmlEnd;

    QDateTime actualDateTime;
};

#endif

