/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 3.1.0-dev
 Written in Qt
 
                         graph.h  -  description
                             -------------------
    copyright         : (C) 2005-2023 by Dimitris B. Kalamaras
    project site      : https://socnetv.org

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
#include <QQueue>
#include <QUrl>
#include <QHash>
#include <QSet>
#include <QMultiHash>
#include <QMultiMap>
#include <QTextStream>
#include <QThread>

// Allows to use QT_CHARTS namespace directives (see below)
#include <QtCharts/QChartGlobal>


//FYI: stack is a wrapper around <deque> in C++, see: www.cplusplus.com/reference/stl/stack
#include <stack>
#include <map>

#include "global.h"
#include "graphvertex.h"
#include "matrix.h"
#include "parser.h"
#include "webcrawler.h"
#include "graphicswidget.h"

class QDateTime;
class QPointF;
class QNetworkReply;

class QAbstractSeries;
class QAbstractAxis;
class QSplineSeries;
class QBarSeries;
class QAreaSeries;
class QBarSet;
class QBarCategoryAxis;


using namespace std;


SOCNETV_USE_NAMESPACE
class Chart;

typedef QList<GraphVertex*> VList;
typedef QHash <QString, int> H_StrToInt;
typedef QHash <int, int> H_Int;
typedef QHash <qreal, int> H_f_i;
typedef QPair <qreal, bool> pair_f_b;
typedef QPair <int, pair_f_b > pair_i_fb;
typedef QMultiHash <int, pair_i_fb > H_edges;
typedef QHash<QString, bool> H_StrToBool;
typedef QList<int> L_int;
typedef QVector<int> V_int;
typedef QVector<QString> V_str;


/**
  KNOWN BUGS:
  \todo Group edge editing: i.e. change weight or color.
  \todo - CHECK isWeighted corner case results, when !graphIsModified.

  \bug Create d-regular, undirected, ask for closeness, it says we are on a disconnected graph
  \bug Cannot read Graphml files where nodes are not declared before edges (i.e. nets/killer.graphml)
  \bug wontfix Pajek files with no ic / labels without quotes are displayed without colors
  \bug wrong default edge colors (not the ones used by Settings) after loading GraphML files.

  \todo Subgroups / Communities: clans, path distance MDS, Components, Blocks and Cutpoints.
  \todo Structural Equivalence: MDS, Block modelling, CONCOR
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
    QThread webcrawlerThread;

public slots:

    int relationCurrent();

    QString relationCurrentName() const;

    void relationCurrentRename(const QString &newName);
    void relationCurrentRename(const QString &newName, const bool &signalMW);

    /** Slots to signals from Parser */
    void vertexCreate (const int &number,
                        const int &size,
                        const QString &color,
                        const QString &numColor,
                        const int &numSize,
                        const QString &label,
                        const QString &labelColor,
                        const int &labelSize,
                        const QPointF &p,
                        const QString &shape,
                        const QString &iconPath = QString(),
                        const bool &signalMW = false
            );

    void graphFileLoaded(const int &fileType,
                         const QString &fName=QString(),
                         const QString &netName=QString(),
                         const int &totalNodes=0,
                         const int &totalLinks=0,
                         const int &edgeDirType=0,
                         const qint64 &elapsedTime=0,
                         const QString &message=QString());

    void vertexRemoveDummyNode(int);

    void graphLoadedTerminateParserThreads (QString reason);

    void setSelectionChanged(const QList<int> selectedVertices,
                               const QList<SelectedEdge> selectedEdges);

    void graphClickedEmptySpace( const QPointF &p);

    /** Slots to signals from GraphicsWidget and Parser*/
    void edgeCreate  (const int &v1, const int &v2, const qreal &weight,
                      const QString &color ,
                      const int &type=0,
                      const bool &drawArrows=true, const bool &bezier=false,
                      const QString &label=QString(),
                      const bool &signalMW=true);

    void edgeCreateWebCrawler (const int &source, const int &target);

    // helper vertexCreate functions
    void vertexCreateAtPos(const QPointF &p);

    void vertexCreateAtPosRandom(const bool &signalMW=false);

    void vertexCreateAtPosRandomWithLabel(const int &i,
                                          const QString &label,
                                          const bool &signalMW=false) ;


    /** Slots to signals from MainWindow */

    void relationSet(int relNum=RAND_MAX, const bool &updateUI=true);

    void relationNext();

    void relationPrev();

    void canvasSizeSet(const int w, const int h);

    double canvasMaxRadius() const;

    qreal canvasMinDimension() const;

    double canvasVisibleX(const double &x) const ;

    double canvasVisibleY(const double &y) const ;

    double canvasRandomX()  const;

    double canvasRandomY()  const;


    void vertexIsolatedAllToggle ( const bool &toggle);

    void vertexClickedSet(const int &v, const QPointF &p);

    void edgeClickedSet(const int &v1, const int &v2, const bool &openMenu=false) ;


    void edgeFilterByWeight (const qreal, const bool);

    void edgeFilterByRelation(int relation, bool status);

    void edgeFilterUnilateral(const bool &toggle);

    void startWebCrawler(
            const QUrl &startUrl,
            const QStringList &urlPatternsIncluded,
            const QStringList &urlPatternsExcluded,
            const QStringList &linkClasses,
            const int &maxNodes,
            const int &maxLinksPerPage,
            const bool &intLinks,
            const bool &childLinks,
            const bool &parentLinks,
            const bool &selfLinks,
            const bool &extLinksIncluded,
            const bool &extLinksCrawl,
            const bool &socialLinks,
            const bool &delayedRequests);

    void slotHandleCrawlerRequestReply();
    void webSpider();

    QString htmlEscaped (QString str) const;


signals:

    void signalWebCrawlParse(QNetworkReply *reply);

    /** Signals to MainWindow */

    void signalNetworkManagerRequest(const QUrl &currentUrl, const NetworkRequestType &type);

    void signalProgressBoxCreate(const int max=0, const QString msg="Please wait");

    void signalProgressBoxKill(const int max=0);

    void signalProgressBoxUpdate(const int &count=0 );

    void signalGraphSavedStatus(const int &status);

    void signalGraphModified(const bool &undirected,
                             const int &vertices,
                             const int &edges,
                             const qreal &density,
                             const bool &notSaved=true);

    void signalGraphLoaded (const int &fileType,
                            const QString &fileName=QString(),
                            const QString &netName=QString(),
                            const int &totalNodes=0,
                            const int &totalLinks=0,
                            const qreal &density=0,
                            const qint64 &elapsedTime=0,
                            const QString &message=QString() );



    void statusMessage (const QString &message);

    void signalDatasetDescription(QString);

    void signalNodeClickedInfo(const int &number=0,
                                    const QPointF &p=QPointF(),
                                    const QString &label=QString(),
                                    const int &inDegree=0,
                                    const int &outDegree=0);


    void signalEdgeClicked (const MyEdge &edge=MyEdge(), const bool &openMenu=false);

    void signalRelationAddToMW(const QString &newRelation);

    void signalRelationsClear();

    void signalRelationRenamedToMW(const QString &newRelName);

    void signalRelationChangedToGW(int);

    void signalRelationChangedToMW(const int &relIndex=RAND_MAX);

    void signalSelectionChanged(const int &selectedVertices,
                                const int &selectedEdges);


    void signalPromininenceDistributionChartUpdate(QAbstractSeries *series,
                                                   QAbstractAxis *axisX=Q_NULLPTR,
                                                   const qreal &min=0,
                                                   const qreal &max=0,
                                                   QAbstractAxis *axisY=Q_NULLPTR,
                                                   const qreal &minF=0,
                                                   const qreal &maxF=0);

    /** Signals to GraphicsWidget */
    void signalDrawNode( const QPointF &p,
                         const int &num,
                         const int &size,
                         const QString &nodeShape,
                         const QString &nodeIconPath,
                         const QString &nodeColor,
                         const QString &numberColor, const int &numSize,
                         const int &numDistance,
                         const QString &label,
                         const QString &labelColor, const int &labelSize,
                         const int &labelDistance
                         );

    //signal to GW to erase a node
    void signalRemoveNode (int );

    //signal GW to draw an edge
    void signalDrawEdge ( const int &v1,
                          const int &v2,
                          const qreal &weight,
                          const QString &label="",
                          const QString &color="black",
                          const int &type=0,
                          const bool &drawArrows=true,
                          const bool &bezier=false,
                          const bool &weightNumbers=false);

    //signal to GW
    void signalRemoveEdge(const int &v1, const int &v2, const bool &removeReverse);

    void signalSetEdgeVisibility (const int &relation, const int &source, const int &target, const bool &toggle, const bool &checkInverse=false);

    void setVertexVisibility(const int &number, const bool &toggle);

    void setNodePos(const int &, const qreal &, const qreal &);

    void signalNodesFound(const QList<int> foundList);

    void setNodeSize(const int &v, const int &size);

    void setNodeShape(const int &v, const QString &shape, const QString &iconPath=QString());

    void setNodeColor(const int &v, const QString &color);

    void setNodeLabel(const int &v, const QString &label);

    void setNodeNumberColor(const int &v, const QString &color);

    void setNodeNumberSize(const int &v, const int &size);

    void setNodeNumberDistance(const int &v, const int &distance);

    void setNodeLabelSize(const int &v, const int &size);

    void setNodeLabelColor(const int &v, const QString &color);

    void setNodeLabelDistance(const int &v, const int &distance);


    void setEdgeWeight (const int &v1, const int &v2, const qreal &weight);
    void signalEdgeType(const int &v1,
                        const int &v2,
                        const int &type);
    void setEdgeColor(const int &v1,
                         const int &v2,
                         const QString &color);
    void setEdgeLabel (const int &v1,
                       const int &v2,
                       const QString &label);
    void addGuideCircle(const double&, const double&, const double&);
    void addGuideHLine (const double &y0);



public:

    enum ModStatus {
        NewNet          = -1,
        SavedUnchanged  = 0,
        MinorOptions    = 1,
        VertexMetadata  = 2,
        EdgeMetadata    = 3,
        VertexPositions = 4,
        MajorChanges    = 10,
        VertexCount     = 11,
        EdgeCount       = 12,
        VertexEdgeCount = 13,
    };



    enum Clustering {
        Single_Linkage   = 0, //"single-link" or minimum
        Complete_Linkage = 1, // "complete-link or maximum
        Average_Linkage  = 2, //mean or "average-linkage" or UPGMA

    };

    /* INIT AND CLEAR*/
    Graph();
    ~Graph();


    void clear(const QString &reason="");

    /*FILES (READ AND WRITE)*/

    QString getFileName() const;

    void setFileName(QString &fileName);

    QString getName() const;

    void setName(QString &graphName);

    bool isSaved() const;

    bool isLoaded() const;

    int getFileFormat() const;

    bool isFileFormatExportSupported(const int &fileFormat) const;

    void setModStatus(const int &graphNewStatus, const bool&signalMW=true);

    bool isModified() const ;

    void loadFile (const QString fileName,
                    const QString codecName,
                    const int format,
                    const int two_sm_mode,
                    const QString delimiter=QString());

    void saveToFile(const QString &fileName,
                   const int &fileType,
                   const bool &saveEdgeWeights=true);

    bool saveToPajekFormat (const QString &fileName,
                                 QString networkName="",
                                 int maxWidth=0, int maxHeight=0);

    bool saveToAdjacencyFormat (const QString &fileName,
                                     const bool &saveEdgeWeights=true);

    bool saveToGraphMLFormat (const QString &fileName,
                                   QString networkName="",
                                   int maxWidth=0, int maxHeight=0);

    bool saveToDotFormat (QString fileName);

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
    QSet<int> vertexNeighborhoodSet(const int &v1);

    bool vertexIsolated(const int &v1) const;

    int vertexExists(const int &v1 );

    int vertexExists(const QString &label);

    bool vertexFindByNumber (const QStringList &numList);

    bool vertexFindByLabel (const QStringList &labelList);

    bool vertexFindByIndexScore(const int &index,
                                const QStringList &thresholds,
                                const bool &considerWeights,
                                const bool &inverseWeights=false,
                                const bool &dropIsolates=false
                                );

    void vertexRemove (const int &v1);

    void vertexSizeInit (const int);

    void vertexSizeSet(const int &v, const int &newsize);

    int vertexSize(const int &v) const;

    void vertexShapeSetDefault (const QString, const QString &iconPath=QString());

    void vertexShapeSet(const int &v, const QString &shape, const QString &iconPath=QString());

    QString vertexShape(const int &v);

    QString vertexShapeIconPath(const int &v);

    bool graphHasVertexCustomIcons () const { return m_graphHasVertexCustomIcons; }

    void vertexColorInit (const QString &color);

    void vertexColorSet(const int &v, const QString &color);

    QColor vertexColor(const int &v) const;

    void vertexNumberColorInit (const QString &color);

    void vertexNumberColorSet(const int &v=0, const QString &color = "#000000" );

    void vertexNumberSizeInit (const int &size);

    void vertexNumberSizeSet(const int &v, const int &newsize );

    void vertexNumberDistanceInit (const int &distance);

    void vertexNumberDistanceSet(const int &v, const int &newDistance );

    void vertexLabelSet(const int &v, const QString &label);

    QString vertexLabel(const int &v) const;

    void vertexLabelsVisibilitySet(bool toggle);

    void vertexLabelSizeInit(int newSize);

    void vertexLabelSizeSet(const int &v, const int &labelSize );

    void vertexLabelColorInit(QString color);

    void vertexLabelColorSet(const int &v1, const QString &color);

    void vertexLabelDistanceInit (const int &distance);

    void vertexLabelDistanceSet(const int &v, const int &newDistance );

    void vertexLabelDistanceAllSet (const int &newDistance);


    void vertexPosSet(const int &v, const int &x, const int &y);

    QPointF vertexPos(const int &v1) const;

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




    /* EDGES */

    int edgesEnabled();

    MyEdge edgeClicked();

    qreal edgeExists(const int &v1,
                     const int &v2,
                     const bool &checkReciprocal=false);

    void edgeRemove (const int &v1,
                     const int &v2,
                     const bool &removeReverse=false);

    void edgeRemoveSelected (SelectedEdge &selectedEdge,
                             const bool &removeReverse);

    void edgeRemoveSelectedAll();

    bool edgeSymmetric(const int &v1, const int &v2);

    void edgeTypeSet(const int &v1,
                     const int &v2,
                     const qreal &w,
                     const int &dirType=EdgeType::Directed);

    void edgeWeightSet (const int &v1,
                        const int &v2,
                        const qreal &w,
                        const bool &undirected=false);

    qreal edgeWeight(const int &v1, const int &v2) const;

    void edgeWeightNumbersVisibilitySet (const bool &toggle);

    void edgeLabelSet(const int &v1, const int &v2, const QString &label);

    QString edgeLabel (const int &v1, const int &v2) const;

    void edgeLabelsVisibilitySet (const bool &toggle);

    void edgeColorInit(const QString &);

    void edgeColorSet(const int &v1, const int &v2, const QString &color);

    QString edgeColor (const int &v1, const int &v2);

    bool edgeColorAllSet(const QString &color, const int &threshold=RAND_MAX);


    /* GRAPH methods */

    bool isEmpty() const;

    QList<int> getSelectedVertices() const;

    int getSelectedVerticesCount() const;

    int getSelectedVerticesMin() const;

    int getSelectedVerticesMax() const;

    QList<SelectedEdge> getSelectedEdges() const;

    int getSelectedEdgesCount() const;

    int getGeodesicsCount();

    qreal graphDensity();

    bool isWeighted();

    void setWeighted(const bool &toggle=true);

    qreal graphReciprocity();

    bool isSymmetric();

    void setSymmetric();

    void addRelationSymmetricStrongTies(const bool &allRelations=false);

    void relationAddCocitation();

    void graphDichotomization(const qreal threshold);

    void setDirected(const bool &toggle=true, const bool &signalMW=true);

    void setUndirected(const bool &toggle=true, const bool &signalMW=true);

    bool isDirected();

    bool isUndirected();

    bool isConnected();

    void createMatrixAdjacency(const bool dropIsolates=false,
                                    const bool considerWeights=true,
                                    const bool inverseWeights=false,
                                    const bool symmetrize=false );

    bool createMatrixAdjacencyInverse(const QString &method="lu");


    void createMatrixSimilarityMatching(Matrix &AM,
                                             Matrix &SEM,
                                             const int &measure=METRIC_SIMPLE_MATCHING,
                                             const QString &varLocation="Rows",
                                             const bool &diagonal=false,
                                             const bool &considerWeights=true);

    void createMatrixSimilarityPearson (Matrix &AM,
                                             Matrix &PCC,
                                             const QString &varLocation="Rows",
                                             const bool &diagonal=false);

    void createMatrixDissimilarities(Matrix &INPUT_MATRIX,
                                          Matrix &DSM,
                                          const int &metric,
                                          const QString &varLocation,
                                          const bool &diagonal,
                                          const bool &considerWeights);

    /* REPORT EXPORTS */
    void setReportsDataDir(const QString &reportsDir);
    void setReportsRealNumberPrecision (const int & precision);
    void setReportsLabelLength(const int &length);
    void setReportsChartType(const int &type);

    void writeDataSetToFile(const QString dir, const QString );

    void writeMatrixAdjacencyTo(QTextStream& os,
                                const bool &saveEdgeWeights=true);

    void writeReciprocity( const QString fileName,
                           const bool considerWeights=false);

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

    void writeEccentricity( const QString fileName,
                            const bool considerWeights=false,
                            const bool inverseWeights=false,
                            const bool dropIsolates=false);

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


    bool writeClusteringHierarchical(const QString &fileName,
                                     const QString &varLocation,
                                     const QString &matrix = "Adjacency",
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

    bool writeCliqueCensus( const QString &fileName,
                            const bool considerWeights);

    void writeClusteringCoefficient(const QString, const bool);

    void writeTriadCensus(const QString, const bool);



    /* DISTANCES, CENTRALITIES & PROMINENCE MEASURES */

    int graphConnectednessFull (const bool updateProgress=false) ;

    bool graphReachable(const int &v1, const int &v2) ;

    void createMatrixReachability() ;

    int graphDiameter(const bool considerWeights, const bool inverseWeights);

    int graphDistanceGeodesic(const int &v1,
                              const int &v2,
                              const bool &considerWeights=false,
                              const bool &inverseWeights=true);

    qreal graphDistanceGeodesicAverage(const bool considerWeights,
                                       const bool inverseWeights,
                                       const bool dropIsolates);

    void graphDistancesGeodesic(const bool &computeCentralities=false,
                                const bool &considerWeights=false,
                                const bool &inverseWeights=true,
                                const bool &dropIsolates=false);

    void graphMatrixDistanceGeodesicCreate(const bool &considerWeights=false,
                                     const bool &inverseWeights=true,
                                     const bool &dropIsolates=false);

    void graphMatrixShortestPathsCreate(const bool &considerWeights=false,
                                        const bool &inverseWeights=true,
                                        const bool &dropIsolates=false) ;

    int getProminenceIndexByName(const QString &prominenceIndexName);

    void prominenceDistribution(const int &index,
                                const ChartType &type,
                                const QString &distImageFileName=QString());

    void prominenceDistributionBars(const H_StrToInt &discreteClasses,
                                    const QString &name,
                                    const QString &distImageFileName);

    void prominenceDistributionArea(const H_StrToInt &discreteClasses,
                                    const QString &name,
                                    const QString &distImageFileName);

    void prominenceDistributionSpline(const H_StrToInt &discreteClasses,
                                      const QString &seriesName,
                                      const QString &distImageFileName);

    void centralityDegree(const bool &considerWeights=true,
                          const bool &dropIsolates=false);

    void centralityInformation(const bool considerWeights=false,
                               const bool inverseWeights=false);

    void centralityEigenvector(const bool &considerWeights=false,
                               const bool &inverseWeights=false,
                               const bool &dropIsolates=false);

    void centralityClosenessIR(const bool considerWeights=false,
                               const bool inverseWeights=false,
                               const bool dropIsolates=false);

    void prestigeDegree(const bool &considerWeights, const bool &dropIsolates=false);

    void prestigePageRank(const bool &dropIsolates=false);

    void prestigeProximity(const bool considerWeights=false,
                           const bool inverseWeights=false,
                           const bool dropIsolates=false);


    /* REACHABILITY AND WALKS */

    int walksBetween(int v1, int v2,int length);

    void graphWalksMatrixCreate(const int &N=0,
                                const int &length=0,
                                const bool &updateProgress=false);

    void writeWalksTotalMatrixPlainText(const QString &fn);

    void writeWalksOfLengthMatrixPlainText(const QString &fn, const int &length);

    void writeMatrixWalks (const QString &fn,
                           const int &length=0,
                           const bool &simpler=false);

    QList<int> vertexinfluenceRange(int v1);

    QList<int> vertexinfluenceDomain(int v2);

    void writeReachabilityMatrixPlainText( const QString &fn,
                                           const bool &dropIsolates=false);


    qreal numberOfTriples(int v1);

    /* CLIQUES, CLUSTERING, TRIADS */
    void graphCliques(QSet<int> R=QSet<int>(), QSet<int> P=QSet<int>(), QSet<int> X=QSet<int>() );

    void graphCliqueAdd (const QList<int> &clique);

    int graphCliquesContaining(const int &actor, const int &size=0);

    int graphCliquesOfSize(const int &size );

    bool graphClusteringHierarchical(Matrix &STR_EQUIV,
                                     const QString &varLocation,
                                     const int &metric,
                                     const int &method,
                                     const bool &diagonal=false,
                                     const bool &diagram=false,
                                     const bool &considerWeights=true,
                                     const bool &inverseWeights=false,
                                     const bool &dropIsolates=false);

    qreal clusteringCoefficientLocal(const int &v1);

    qreal clusteringCoefficient (const bool updateProgress=false);

    bool graphTriadCensus();

    void triadType_examine_MAN_label(int, int, int, GraphVertex*,  GraphVertex*, GraphVertex* );
    //	void eccentr_JordanCenter();    // TODO



    /* LAYOUTS */

    void layoutRandom();

    void layoutRadialRandom(const bool &guides=true);

    void layoutCircular(const double &x0,
                        const double &y0,
                        const double &newRadius,
                        const bool &guides=false);


    void layoutByProminenceIndex ( int prominenceIndex,
                                   int layoutType,
                                   const bool &considerWeights=false,
                                   const bool &inverseWeights=false,
                                   const bool &dropIsolates=false);


    void layoutVertexSizeByIndegree();

    void layoutVertexSizeByOutdegree();


    void layoutForceDirectedSpringEmbedder(const int maxIterations);

    void layoutForceDirectedFruchtermanReingold(const int maxIterations);

    void layoutForceDirectedKamadaKawai(const int maxIterations=500,
                                        const bool considerWeights=false,
                                        const bool inverseWeights=false,
                                        const bool dropIsolates=false,
                                        const QString &initialPositions="current");

    qreal graphDistanceEuclidean(const QPointF &a, const QPointF &b);

    qreal graphDistanceEuclidean(const QPointF &a);

    int sign(const qreal &D);

    qreal layoutForceDirected_F_rep(const QString model,
                                    const qreal &dist,
                                    const qreal &optimalDistance);

    qreal layoutForceDirected_F_att(const QString model,
                                    const qreal &dist,
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



    /* CRAWLER */
    void webCrawlTerminateThreads (QString reason);



    /**RANDOM NETWORKS*/
    void randomizeThings();

    void randomNetErdosCreate (  const int &N,
                                 const QString &model,
                                 const int &m,
                                 const qreal &p,
                                 const QString &mode,
                                 const bool &diag);

    void randomNetScaleFreeCreate (const int &N,
                                    const int &power,
                                    const int &m0,
                                    const int &m,
                                    const qreal &alpha,
                                    const QString &mode);

    void randomNetSmallWorldCreate(const int &N, const int &degree,
                                   const double &beta, const QString &mode);

    void randomNetRingLatticeCreate (const int &N, const int &degree,
                                      const bool updateProgress=false);

    void randomNetRegularCreate (const int &N,
                                    const int &degree,
                                    const QString &mode,
                                    const bool &diag);

    void randomNetLatticeCreate(const int &N,
                                const int &length,
                                const int &dimension,
                                const int &neighborhoodLength,
                                const QString &mode,
                                const bool &circular=false);

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



private:

    /** private member functions */

    void edgeAdd (const int &v1,
                  const int &v2,
                  const qreal &weight,
                  const int &type,
                  const QString &label,
                  const QString &color
                  );

    /** methods used by graphDistancesGeodesic()  */
    void BFS(const int &s,
             const int &si,
             const bool &computeCentralities=false,
             const bool &dropIsolates=false);

    void dijkstra(const int &s,
                  const int &si,
                  const bool &computeCentralities=false,
                  const bool &inverseWeights=false,
                  const bool &dropIsolates=false);

    void minmax(qreal C,
                GraphVertex *v,
                qreal &max,
                qreal &min,
                int &maxNode, int &minNode
              );

    void resolveClasses ( qreal C,
                          H_StrToInt &discreteClasses,
                          int &classes);

    void resolveClasses ( qreal C,
                           H_StrToInt &discreteClasses,
                           int &classes, int name);


    VList m_graph;                              // List of pointers to the vertices. Each vertex stores all info: links, colors, etc

    Parser *file_parser;                        // Our file loader threaded class.

    WebCrawler *web_crawler;                     // Our web crawler threaded class. This will parse the downloaded HTML.

    QQueue<QUrl> *urlQueue;                     // A queue where the crawler will put urls for the network manager to download

    int m_crawler_max_urls;                      // maximum urls we'll visit (max nodes in the resulted network)
    int m_crawler_visited_urls;                  // A counter of the urls visited.


    QList<QString> m_relationsList;

    QList<int> m_graphFileFormatExportSupported;

    QList<int> triadTypeFreqs;                  //stores triad type frequencies

    QList<int> m_verticesList;
    QList<int> m_verticesIsolatedList;

    QList<int> m_verticesSelected;

    QSet<int> m_verticesSet;

    QList<SelectedEdge> m_selectedEdges;

    QMultiHash <int, int> influenceRanges, influenceDomains;

    QMultiHash <int, int> m_vertexPairsNotConnected;
    QHash <int, int> m_vertexPairsUnilaterallyConnected;

    QMultiMap <int, L_int > m_cliques;
    QHash <int, QSet<int> > neighboursHash;

    QList <qreal> m_clusteringLevel;
    QMap <int, V_int> m_clustersPerSequence;

    QMap<QString, V_int> m_clustersByName;
    QMap<int, V_str> m_clusterPairNamesPerSeq;

    Matrix  SIGMA, DM, sumM, invAM, AM, invM, WM;
    Matrix XM, XSM, XRM, CLQM;

    stack<int> Stack;

    /** used in resolveClasses and graphDistancesGeodesic() */
    H_StrToInt discreteDPs, discreteSDCs, discreteCCs, discreteBCs, discreteSCs;
    H_StrToInt discreteIRCCs, discreteECs, discreteEccentricities;
    H_StrToInt discretePCs, discreteICs,  discretePRPs, discretePPs, discreteEVCs;

    QString m_reportsDataDir;
    int m_reportsRealPrecision;
    int m_reportsLabelLength;
    ChartType m_reportsChartType;

    int m_fieldWidth, m_curRelation, m_fileFormat, m_vertexClicked;

    MyEdge m_clickedEdge;

    qreal edgeWeightTemp, edgeReverseWeightTemp;
    qreal meanSDC, varianceSDC;
    qreal meanSCC, varianceSCC;
    qreal meanIRCC, varianceIRCC;
    qreal meanSBC, varianceSBC;
    qreal meanSSC, varianceSSC;
    qreal meanEC, varianceEC;
    qreal meanSPC, varianceSPC;
    qreal meanIC, varianceIC;
    qreal meanEVC, varianceEVC;
    qreal meanSDP, varianceSDP;
    qreal meanPP, variancePP;
    qreal meanPRP, variancePRP;
    qreal minEccentricity, maxEccentricity;
    qreal minSDP, maxSDP, sumDP, sumSDP, groupDP;
    qreal minSDC, maxSDC, sumDC, sumSDC, groupDC;
    qreal minSCC, maxSCC, nomSCC, denomSCC, sumCC, sumSCC, groupCC, maxIndexCC;
    qreal minIRCC, maxIRCC, nomIRCC, denomIRCC, sumIRCC, groupIRCC;
    qreal minSBC, maxSBC, nomSBC, denomSBC, sumBC, sumSBC, groupSBC, maxIndexBC;
    qreal minSPC, maxSPC, nomSPC, denomSPC, t_sumIC, sumSPC, groupSPC, maxIndexPC;
    qreal minSSC, maxSSC, sumSC, sumSSC, groupSC, maxIndexSC;
    qreal minEC, maxEC, nomEC, denomEC, sumEC, groupEC, maxIndexEC;
    qreal minIC, maxIC, nomIC, denomIC, sumIC, maxIndexIC;
    qreal minEVC, maxEVC, nomEVC, denomEVC, sumEVC, sumSEVC, groupEVC;
    qreal minPRP, maxPRP, nomPRC, denomPRC, sumPC, t_sumPRP, sumPRP;
    qreal minPP, maxPP, nomPP, denomPP, sumPP, groupPP;

    qreal minCLC, maxCLC, averageCLC,varianceCLC, d_factor;
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
    qreal sizeOfComponent;

    /** General & initialisation variables */

    int m_graphModStatus;
    int m_totalVertices, m_totalEdges, m_graphDiameter, initVertexSize;
    int initVertexLabelSize, initVertexNumberSize;
    int initVertexNumberDistance, initVertexLabelDistance;
    bool order;
    bool initEdgeWeightNumbers, initEdgeLabels;
    qreal m_graphAverageDistance, m_graphGeodesicsCount;
    qreal m_graphDensity;
    qreal m_graphSumDistance;
    qreal m_graphReciprocityArc, m_graphReciprocityDyad;
    int m_graphReciprocityTiesReciprocated;
    int m_graphReciprocityTiesNonSymmetric;
    int m_graphReciprocityTiesTotal;
    int m_graphReciprocityPairsReciprocated;
    int m_graphReciprocityPairsTotal;

    bool m_graphHasVertexCustomIcons;

    int outboundEdgesVert, inboundEdgesVert, reciprocalEdgesVert;

    qreal canvasWidth, canvasHeight;
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
    bool m_graphIsDirected, m_graphIsSymmetric, m_graphIsWeighted, m_graphIsConnected;

    int csRecDepth;

    QString m_fileName, m_graphName, initEdgeColor, initVertexColor,
        initVertexNumberColor, initVertexLabelColor;
    QString initVertexShape, initVertexIconPath;
    QString htmlHead, htmlHeadLight, htmlEnd;

    QDateTime actualDateTime;
};

#endif

