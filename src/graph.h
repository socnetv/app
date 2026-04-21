/**
 * @file graph.h
 * @brief Defines the Graph class and related algorithms for social network visualization.
 * @author Dimitris B. Kalamaras
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

#ifndef GRAPH_H
#define GRAPH_H

#include <QObject>
#include <QList>
#include <QQueue>
#include <QHash>
#include <QSet>
#include <QMultiHash>
#include <QMultiMap>
#include <QTextStream>
#include <QThread>
#include <QStack>

// stack is a wrapper around <deque> in C++
// see: www.cplusplus.com/reference/stl/stack
#include <stack>

#include "global.h"
#include "graphvertex.h"
#include "matrix.h"
#include "parser.h"
#include "webcrawler.h"

class QDateTime;
class QPointF;
class QNetworkReply;
class QUrl;
class QAbstractSeries;
class QAbstractAxis;

using namespace std;

SOCNETV_USE_NAMESPACE
class Chart;

typedef QList<GraphVertex *> VList;
typedef QHash<QString, int> H_StrToInt;
typedef QHash<int, int> H_Int;
typedef QHash<qreal, int> H_f_i;
typedef QPair<qreal, bool> pair_f_b;
typedef QPair<int, pair_f_b> pair_i_fb;
typedef QMultiHash<int, pair_i_fb> H_edges;
typedef QHash<QString, bool> H_StrToBool;
typedef QList<int> L_int;
typedef QList<int> V_int;
typedef QList<QString> V_str;

/**
 * @brief The Graph class
 * This is the main class for a Graph, used in conjuction with GraphVertex, Parser and Matrix objects.
 *   Graph class methods are the interface to various analysis algorithms
 *   GraphVertex class holds each vertex data (colors, strings, statistics, etc)
 *   Matrix class holds the adjacency matrix of the network.
 *   Parser class loads files of networks.
 */
class Graph : public QObject
{
    Q_OBJECT
    QThread file_parserThread;
    QThread webcrawlerThread;

    friend class DistanceEngine;

public slots:
    // ============================================================================
    // LEGACY/INTERNAL (UI / IO WIRING)
    // ----------------------------------------------------------------------------
    // NOTE (WS2/F0):
    // Slots are part of Graph's coordinator role (signals/threads/UI wiring).
    // Engines/services should not depend on these directly.
    // ============================================================================

    int relationCurrent();

    QString relationCurrentName() const;

    void relationCurrentRename(const QString &newName);
    void relationCurrentRename(const QString &newName, const bool &signalMW);

    /** Slots to signals from Parser */
    void vertexCreate(const int &number,
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
                      const bool &signalMW = false,
                      const QHash<QString, QString> &customAttributes = QHash<QString, QString>());

    void graphFileLoaded(const int &fileType,
                         const QString &fName = QString(),
                         const QString &netName = QString(),
                         const int &totalNodes = 0,
                         const int &totalLinks = 0,
                         const int &edgeDirType = 0,
                         const qint64 &elapsedTime = 0,
                         const QString &message = QString());

    void vertexRemoveDummyNode(int);

    void graphLoadedTerminateParserThreads(QString reason);

    void setSelectionChanged(const QList<int> selectedVertices,
                             const QList<SelectedEdge> selectedEdges);

    void graphClickedEmptySpace(const QPointF &p);

    /** Slots to signals from GraphicsWidget and Parser*/
    bool edgeCreate(const int &v1, const int &v2, const qreal &weight,
                    const QString &color,
                    const int &type = 0,
                    const bool &drawArrows = true, const bool &bezier = false,
                    const QString &label = QString(),
                    const bool &signalMW = true,
                    const QHash<QString,QString> &edgeCustomAttributes =
                        QHash<QString,QString>());

    void edgeCreateWebCrawler(const int &source, const int &target);

    // helper vertexCreate functions
    void vertexCreateAtPos(const QPointF &p);

    void vertexCreateAtPosRandom(const bool &signalMW = false);

    void vertexCreateAtPosRandomWithLabel(const int &i,
                                          const QString &label,
                                          const bool &signalMW = false);

    /** Slots to signals from MainWindow */

    void relationSet(int relNum = RAND_MAX, const bool &updateUI = true);

    void relationNext();

    void relationPrev();

    void canvasSizeSet(const int &width, const int &height);

    double canvasMaxRadius() const;

    qreal canvasMinDimension() const;

    double canvasVisibleX(const double &x) const;

    double canvasVisibleY(const double &y) const;

    double canvasRandomX() const;

    double canvasRandomY() const;

    void vertexIsolatedAllToggle(const bool &toggle);

    void vertexClickedSet(const int &v, const QPointF &p);

    void edgeClickedSet(const int &v1, const int &v2, const bool &openMenu = false);

    void vertexFilterByCentrality(const float threshold,
                                  const bool overThreshold,
                                  const IndexType centralityIndex);

    void vertexFilterByEgoNetwork(const int v1, const int depth = 1);
    void vertexFilterBySelection(const QList<int> &selectedVertices);
    void vertexFilterByAttribute(const QString &key, const QString &value);
    void vertexFilterRestoreAll();
    bool visibilityHistoryEmpty() const;

    void edgeFilterByWeight(const qreal, const bool);
    void edgeFilterReset();

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

    void slotCancelComputation();

    QString htmlEscaped(QString str) const;

signals:

    void signalWebCrawlParse(QNetworkReply *reply);

    // ============================================================================
    // LEGACY/INTERNAL (UI SIGNAL SURFACE)
    // ----------------------------------------------------------------------------
    // NOTE (WS2/F0): Signals are a UI orchestration mechanism. Engines/services
    // must not emit/call UI-facing behavior directly.
    // ============================================================================

    /** Signals to MainWindow */

    void signalNetworkManagerRequest(const QUrl &currentUrl, const NetworkRequestType &type);

    void signalProgressBoxCreate(const int max = 0, const QString msg = "Please wait");

    void signalProgressBoxKill(const int max = 0);

    void signalProgressBoxUpdate(const int &count = 0);

    void signalGraphSavedStatus(const int &status);

    void signalGraphModified(const bool &undirected,
                             const int &vertices,
                             const int &edges,
                             const qreal &density,
                             const bool &notSaved = true);

    void signalGraphLoaded(const int &fileType,
                           const QString &fileName = QString(),
                           const QString &netName = QString(),
                           const int &totalNodes = 0,
                           const int &totalLinks = 0,
                           const qreal &density = 0,
                           const qint64 &elapsedTime = 0,
                           const QString &message = QString());

    void statusMessage(const QString &message);

    void signalDatasetDescription(QString);

    void signalNodeClickedInfo(const int &number = 0,
                               const QPointF &p = QPointF(),
                               const QString &label = QString(),
                               const int &inDegree = 0,
                               const int &outDegree = 0);

    void signalEdgeClicked(const MyEdge &edge = MyEdge(), const bool &openMenu = false);

    void signalRelationAddToMW(const QString &newRelation);

    void signalRelationsClear();

    void signalRelationRenamedToMW(const QString &newRelName);

    void signalRelationChangedToGW(int);

    void signalRelationChangedToMW(const int &relIndex = RAND_MAX);

    void signalSelectionChanged(const int &selectedVertices,
                                const int &selectedEdges);

    void signalPromininenceDistributionChartUpdate(QAbstractSeries *series,
                                                   QAbstractAxis *axisX = Q_NULLPTR,
                                                   const qreal &min = 0,
                                                   const qreal &max = 0,
                                                   QAbstractAxis *axisY = Q_NULLPTR,
                                                   const qreal &minF = 0,
                                                   const qreal &maxF = 0);

    /** Signals to GraphicsWidget */
    void signalDrawNode(const QPointF &p,
                        const int &num,
                        const int &size,
                        const QString &nodeShape,
                        const QString &nodeIconPath,
                        const QString &nodeColor,
                        const QString &numberColor, const int &numSize,
                        const int &numDistance,
                        const QString &label,
                        const QString &labelColor, const int &labelSize,
                        const int &labelDistance);

    // signal to GW to erase a node
    void signalRemoveNode(int);

    // signal GW to draw an edge
    void signalDrawEdge(const int &v1,
                        const int &v2,
                        const qreal &weight,
                        const QString &label = "",
                        const QString &color = "black",
                        const int &type = 0,
                        const bool &drawArrows = true,
                        const bool &bezier = false,
                        const bool &weightNumbers = false);

    // signal to GW
    void signalRemoveEdge(const int &v1, const int &v2, const bool &removeReverse);

    void signalSetEdgeVisibility(const int &relation,
                                 const int &source,
                                 const int &target,
                                 const bool &toggle,
                                 const bool &preserveReverseEdge = false,
                                 const int &edgeWeight = 1,
                                 const int &reverseEdgeWeight = 1); // The last two are used only if we need to draw the edge

    void setVertexVisibility(const int &number, const bool &toggle);

    void setNodePos(const int &, const qreal &, const qreal &);

    void signalNodesFound(const QList<int> foundList);

    void setNodeSize(const int &v, const int &size);

    void setNodeShape(const int &v, const QString &shape, const QString &iconPath = QString());

    void setNodeColor(const int &v, const QString &color);

    void setNodeLabel(const int &v, const QString &label);

    void setNodeNumberColor(const int &v, const QString &color);

    void setNodeNumberSize(const int &v, const int &size);

    void setNodeNumberDistance(const int &v, const int &distance);

    void setNodeLabelSize(const int &v, const int &size);

    void setNodeLabelColor(const int &v, const QString &color);

    void setNodeLabelDistance(const int &v, const int &distance);

    void setEdgeWeight(const int &v1, const int &v2, const qreal &weight);
    void signalEdgeType(const int &v1,
                        const int &v2,
                        const int &type);
    void setEdgeColor(const int &v1,
                      const int &v2,
                      const QString &color);
    void setEdgeLabel(const int &v1,
                      const int &v2,
                      const QString &label);
    void addGuideCircle(const double &, const double &, const double &);
    void addGuideHLine(const double &y0);

public:
    // ============================================================================
    // GRAPH FACADE CONTRACT (WS2 / F0)
    // ----------------------------------------------------------------------------
    // This section defines the *supported* API surface that UI and CLI code may
    // call going forward.
    //
    // Rules:
    // - New UI features MUST use only the "FACADE API (SUPPORTED)" surface.
    // - Engines/services MUST NOT call UI-oriented slots/signals.
    // - Anything explicitly marked LEGACY/INTERNAL is not allowed for new code,
    //   even if it remains public for historical reasons.
    //
    // ============================================================================

    enum ModStatus
    {
        NewNet = -1,
        SavedUnchanged = 0,
        MinorOptions = 1,
        VertexMetadata = 2,
        EdgeMetadata = 3,
        VertexPositions = 4,
        MajorChanges = 10,
        VertexCount = 11,
        EdgeCount = 12,
        VertexEdgeCount = 13,
    };

    enum Clustering
    {
        Single_Linkage = 0,   //"single-link" or minimum
        Complete_Linkage = 1, // "complete-link or maximum
        Average_Linkage = 2,  // mean or "average-linkage" or UPGMA

    };

    // --------------------------------------------------------------------------
    // FACADE API (SUPPORTED): Visibility snapshot for non-destructive filtering.
    // Used by the filter system to save/restore vertex and edge visibility state.
    // Stored as a stack to support future undo/redo.
    // --------------------------------------------------------------------------
    struct GraphVisibilitySnapshot
    {
        QHash<int, bool> nodeVisible;            // vertex number  → was enabled
        QHash<QPair<int, int>, bool> arcVisible; // (source,target)→ was visible
        bool active = false;                     // true when this snapshot holds real data
    };

    /* INIT AND CLEAR*/
    // --------------------------------------------------------------------------
    // FACADE API (SUPPORTED): Lifecycle
    // --------------------------------------------------------------------------
    Graph(const int &reserveVerticesSize = 5000, const int &reserveEdgesPerVertexSize = 500);
    ~Graph();

    QThread *getThread() const;
    void moveToThreadFacade(QThread *thread);

    void clear(const QString &reason = "");

    /*FILES (READ AND WRITE)*/
    // --------------------------------------------------------------------------
    // FACADE API (SUPPORTED): File identity / load-save
    // --------------------------------------------------------------------------

    QString getFileName() const;

    void setFileName(const QString &fileName);

    QString getName() const;

    void setName(const QString &graphName);

    bool isSaved() const;

    bool isLoaded() const;

    int getFileFormat() const;

    void setFileFormat(const int &fileFormat);

    bool isFileFormatExportSupported(const int &fileFormat) const;

    void setModStatus(const int &graphNewStatus, const bool &signalMW = true);

    bool isModified() const;

    void loadFile(const QString fileName,
                  const QString codecName,
                  const int format,
                  const QString delimiter = QString(),
                  const int sm_two_mode = 1,
                  const bool sm_has_labels = false);

    void saveToFile(const QString &fileName,
                    const int &fileType,
                    const bool &saveEdgeWeights = true,
                    const bool &saveZeroWeightEdges = false);

    bool saveToPajekFormat(const QString &fileName,
                           QString networkName = "",
                           int maxWidth = 0, int maxHeight = 0);

    bool saveToAdjacencyFormat(const QString &fileName,
                               const bool &saveEdgeWeights = true);

    bool saveToGraphMLFormat(const QString &fileName,
                             const bool &saveZeroWeightEdges = false,
                             QString networkName = "",
                             int maxWidth = 0, int maxHeight = 0);

    bool saveToDotFormat(QString fileName);

    QString graphMatrixTypeToString(const int &matrixType) const;

    int graphMatrixStrToType(const QString &matrix) const;

    QString graphMetricTypeToString(const int &metricType) const;

    int graphMetricStrToType(const QString &metricStr) const;

    QString graphClusteringMethodTypeToString(const int &methodType) const;

    int graphClusteringMethodStrToType(const QString &method) const;

    /* RELATIONS */
    // --------------------------------------------------------------------------
    // FACADE API (SUPPORTED): Relations
    // --------------------------------------------------------------------------

    int relations();

    void relationsClear();

    void relationAdd(const QString &relName, const bool &changeRelation = false);

    /* VERTICES */
    // --------------------------------------------------------------------------
    // FACADE API (SUPPORTED): Vertex queries + edits
    // --------------------------------------------------------------------------

    int vertexIndexByNumber(int v) const;
    // LEGACY/INTERNAL (ENGINE SUPPORT):
    // Access a vertex by internal storage index (vpos result).
    // No bounds checks: preserves existing behavior of direct m_graph[idx] usage.
    GraphVertex *vertexAtIndex(int idx);
    const GraphVertex *vertexAtIndex(int idx) const;

    VList::const_iterator verticesBegin() const;
    VList::const_iterator verticesEnd() const;

    int vertexNumberMax();

    int vertexNumberMin();

    int vertexDegreeOut(int);

    int vertexDegreeIn(int);

    QList<int> vertexReciprocalNeighborsList(const int &v1);
    QSet<int> vertexReciprocalNeighborsSet(const int &v1);
    QSet<int> vertexOutNeighborsSet(const int &v1, const bool includeInEdges = false);
    
    bool vertexIsolated(const int &v1) const;

    int vertexExists(const int &v1);

    int vertexExists(const QString &label);

    bool vertexFindByNumber(const QStringList &numList);

    bool vertexFindByLabel(const QStringList &labelList);

    bool vertexFindByIndexScore(const int &index,
                                const QStringList &thresholds,
                                const bool &considerWeights,
                                const bool &inverseWeights = false,
                                const bool &dropIsolates = false);

    void vertexRemove(const int &v1);

    void vertexSizeInit(const int);

    void vertexSizeSet(const int &v, const int &newsize);

    int vertexSize(const int &v) const;

    void vertexShapeSetDefault(const QString, const QString &iconPath = QString());

    void vertexShapeSet(const int &v, const QString &shape, const QString &iconPath = QString());

    QString vertexShape(const int &v);

    QString vertexShapeIconPath(const int &v);

    bool graphHasVertexCustomIcons() const;

    QStringList graphHasVertexCustomAttributes() const;

    void vertexColorInit(const QString &color);

    void vertexColorSet(const int &v, const QString &color);

    QColor vertexColor(const int &v) const;

    void vertexNumberColorInit(const QString &color);

    void vertexNumberColorSet(const int &v = 0, const QString &color = "#000000");

    void vertexNumberSizeInit(const int &size);

    void vertexNumberSizeSet(const int &v, const int &newsize);

    void vertexNumberDistanceInit(const int &distance);

    void vertexNumberDistanceSet(const int &v, const int &newDistance);

    void vertexLabelSet(const int &v, const QString &label);

    QString vertexLabel(const int &v) const;

    void vertexLabelsVisibilitySet(bool toggle);

    void vertexLabelSizeInit(int newSize);

    void vertexLabelSizeSet(const int &v, const int &labelSize);

    void vertexLabelColorInit(QString color);

    void vertexLabelColorSet(const int &v1, const QString &color);

    void vertexLabelDistanceInit(const int &distance);

    void vertexLabelDistanceSet(const int &v, const int &newDistance);

    void vertexLabelDistanceAllSet(const int &newDistance);

    void vertexCustomAttributesSet(const int &v1, const QHash<QString, QString> &customAttributes);

    void vertexCustomAttributeSet(const int &v1, const QString &key, const QString &value);

    void vertexCustomAttributeRemove(const int &v1, const QString &key);

    QHash<QString, QString> vertexCustomAttributes(const int &v1) const;

    void vertexPosSet(const int &v, const int &x, const int &y);

    QPointF vertexPos(const int &v1) const;

    int vertexClicked() const;

    int vertices(const bool &dropIsolates = false, const bool &countAll = false, const bool &recount = false);

    int vertexEdgesOutbound(int i);

    int vertexEdgesInbound(int i);

    int verticesWithOutboundEdges();

    int verticesWithInboundEdges();

    int verticesWithReciprocalEdges();

    QList<int> verticesListIsolated();

    QList<int> verticesList();

    QSet<int> verticesSet();

    void verticesCreateSubgraph(QList<int> vList,
                                const int &type = SUBGRAPH_CLIQUE,
                                const int &center = 0);

    // Regression/testing helper: access a vertex object by its number.
    // Returns nullptr if not found.
    GraphVertex *vertexPtr(const int v);

    /* EDGES */
    // --------------------------------------------------------------------------
    // FACADE API (SUPPORTED): Edge queries + edits
    // --------------------------------------------------------------------------

    int edgesEnabled();

    MyEdge edgeClicked();

    qreal edgeExists(const int &v1,
                     const int &v2,
                     const bool &checkReciprocal = false);

    qreal edgeExistsVirtual(const int &v1, const int &v2);

    void edgeOutboundStatusSet(const int &source,
                               const int &target,
                               const bool &toggle = false);

    void edgeInboundStatusSet(const int &target,
                              const int &source,
                              const bool &toggle = false);

    void edgeRemove(const int &v1,
                    const int &v2,
                    const bool &removeReverse = false);

    void edgeRemoveSelected(SelectedEdge &selectedEdge,
                            const bool &removeReverse);

    void edgeRemoveSelectedAll();

    bool edgeSymmetric(const int &v1, const int &v2);

    void edgeTypeSet(const int &v1,
                     const int &v2,
                     const qreal &w,
                     const int &dirType = EdgeType::Directed);

    void edgeWeightSet(const int &v1,
                       const int &v2,
                       const qreal &w,
                       const bool &undirected = false);

    qreal edgeWeight(const int &v1, const int &v2) const;

    void edgeWeightNumbersVisibilitySet(const bool &toggle);

    void edgeLabelSet(const int &v1, const int &v2, const QString &label);

    QString edgeLabel(const int &v1, const int &v2) const;

    void edgeLabelsVisibilitySet(const bool &toggle);

    void edgeColorInit(const QString &);

    void edgeColorSet(const int &v1, const int &v2, const QString &color);

    QString edgeColor(const int &v1, const int &v2);

    bool edgeColorAllSet(const QString &color, const int &threshold = RAND_MAX);

    void edgeCustomAttributesSet(const int &v1, const int &v2, const QHash<QString,QString> &attrs);

    QHash<QString,QString> edgeCustomAttributes(const int &v1, const int &v2) const;

    QStringList graphHasEdgeCustomAttributes() const;

    /* GRAPH methods */
    // --------------------------------------------------------------------------
    // FACADE API (SUPPORTED): Graph facts + settings used by UI/CLI
    // --------------------------------------------------------------------------

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

    void setWeighted(const bool &toggle = true);

    qreal graphReciprocity();

    bool isSymmetric();

    void setSymmetric();

    void addRelationSymmetricStrongTies(const bool &allRelations = false);

    void relationAddCocitation();

    void graphDichotomization(const qreal threshold);

    void setDirected(const bool &toggle = true, const bool &signalMW = true);

    void setUndirected(const bool &toggle = true, const bool &signalMW = true);

    bool isDirected();

    bool isUndirected();

    bool isConnected();

    bool isConnectedCached() const;

    void createMatrixAdjacency(const bool dropIsolates = false,
                               const bool considerWeights = true,
                               const bool inverseWeights = false,
                               const bool symmetrize = false);

    bool createMatrixAdjacencyInverse(const QString &method = "lu");

    void createMatrixSimilarityMatching(Matrix &AM,
                                        Matrix &SEM,
                                        const int &measure = METRIC_SIMPLE_MATCHING,
                                        const QString &varLocation = "Rows",
                                        const bool &diagonal = false,
                                        const bool &considerWeights = true);

    void createMatrixSimilarityPearson(Matrix &AM,
                                       Matrix &PCC,
                                       const QString &varLocation = "Rows",
                                       const bool &diagonal = false);

    void createMatrixDissimilarities(Matrix &INPUT_MATRIX,
                                     Matrix &DSM,
                                     const int &metric,
                                     const QString &varLocation,
                                     const bool &diagonal,
                                     const bool &considerWeights);

    /* REPORT EXPORTS */
    void setReportsDataDir(const QString &reportsDir);
    void setReportsRealNumberPrecision(const int &precision);
    void setReportsLabelLength(const int &length);
    void setReportsChartType(const int &type);

    void writeDataSetToFile(const QString dir, const QString);

    void writeMatrixAdjacencyTo(QTextStream &os,
                                const bool &saveEdgeWeights = true);

    void writeReciprocity(const QString fileName,
                          const bool considerWeights = false);

    bool writeMatrix(const QString &fileName,
                     const int &matrix = MATRIX_ADJACENCY,
                     const bool &considerWeights = true,
                     const bool &inverseWeights = false,
                     const bool &dropIsolates = false,
                     const QString &varLocation = "Rows",
                     const bool &simpler = false);

    void writeMatrixHTMLTable(QTextStream &outText, Matrix &M,
                              const bool &markDiag = true,
                              const bool &plain = false,
                              const bool &printInfinity = true,
                              const bool &dropIsolates = false);

    void writeMatrixAdjacency(const QString fileName,
                              const bool &markDiag = true);

    void writeMatrixAdjacencyPlot(const QString fileName,
                                  const bool &simpler = false);

    bool writeMatrixDissimilarities(const QString fileName,
                                    const QString &metricStr,
                                    const QString &varLocation,
                                    const bool &diagonal,
                                    const bool &considerWeights);

    bool writeMatrixSimilarityMatching(const QString fileName,
                                       const QString &measure = "Simple",
                                       const QString &matrix = "adjacency",
                                       const QString &varLocation = "rows",
                                       const bool &diagonal = false,
                                       const bool &considerWeights = true);

    bool writeMatrixSimilarityPearson(const QString fileName,
                                      const bool considerWeights,
                                      const QString &matrix = "adjacency",
                                      const QString &varLocation = "rows",
                                      const bool &diagonal = false);

    bool writeEccentricity(const QString fileName,
                           const bool considerWeights = false,
                           const bool inverseWeights = false,
                           const bool dropIsolates = false);

    //   friend QTextStream& operator <<  (QTextStream& os, Graph& m);

    bool writeCentralityDegree(const QString,
                               const bool weights,
                               const bool dropIsolates);

    bool writeCentralityCloseness(const QString,
                                  const bool weights,
                                  const bool inverseWeights,
                                  const bool dropIsolates);

    bool writeCentralityClosenessInfluenceRange(const QString,
                                                const bool weights,
                                                const bool inverseWeights,
                                                const bool dropIsolates);

    bool writeCentralityBetweenness(const QString,
                                    const bool weights,
                                    const bool inverseWeights,
                                    const bool dropIsolates);

    bool writeCentralityPower(const QString,
                              const bool weigths,
                              const bool inverseWeights,
                              const bool dropIsolates);

    bool writeCentralityStress(const QString,
                               const bool weigths,
                               const bool inverseWeights,
                               const bool dropIsolates);

    bool writeCentralityEccentricity(const QString,
                                     const bool weigths,
                                     const bool inverseWeights,
                                     const bool dropIsolates);

    bool writeCentralityInformation(const QString,
                                    const bool weigths,
                                    const bool inverseWeights);

    bool writeCentralityEigenvector(const QString,
                                    const bool &weigths = true,
                                    const bool &inverseWeights = false,
                                    const bool &dropIsolates = false);

    bool writePrestigeDegree(const QString, const bool weights,
                             const bool dropIsolates);

    bool writePrestigeProximity(const QString, const bool weights,
                                const bool inverseWeights,
                                const bool dropIsolates);

    bool writePrestigePageRank(const QString, const bool Isolates = false);

    bool writeClusteringHierarchical(const QString &fileName,
                                     const QString &varLocation,
                                     const QString &matrix = "Adjacency",
                                     const QString &metric = "Manhattan",
                                     const QString &method = "Complete",
                                     const bool &diagonal = false,
                                     const bool &dendrogram = false,
                                     const bool &considerWeights = true,
                                     const bool &inverseWeights = false,
                                     const bool &dropIsolates = false);

    void writeClusteringHierarchicalResultsToStream(QTextStream &outText,
                                                    const int N,
                                                    const bool &dendrogram = false);

    bool writeCliqueCensus(const QString &fileName,
                           const bool considerWeights);

    bool writeClusteringCoefficient(const QString, const bool);

    bool writeTriadCensus(const QString, const bool);

    /* DISTANCES, CENTRALITIES & PROMINENCE MEASURES */

    int graphConnectednessFull(const bool updateProgress = false);

    bool graphReachable(const int &v1, const int &v2);

    void createMatrixReachability();

    int graphDiameter(const bool considerWeights, const bool inverseWeights);

    int graphDiameterCached() const;

    qreal graphSumDistanceCached() const;

    qreal graphGeodesicsCountCached() const;

    int graphDistanceGeodesic(const int &v1,
                              const int &v2,
                              const bool &considerWeights = false,
                              const bool &inverseWeights = true);

    qreal graphDistanceGeodesicAverage(const bool considerWeights,
                                       const bool inverseWeights,
                                       const bool dropIsolates);

    qreal graphDistanceGeodesicAverageCached() const;

    void graphDistancesGeodesic(const bool &computeCentralities = false,
                                const bool &considerWeights = false,
                                const bool &inverseWeights = true,
                                const bool &dropIsolates = false);

    // ============================================================================
    // LEGACY/INTERNAL (ENGINE SUPPORT PRIMITIVES)
    // ----------------------------------------------------------------------------
    // NOTE (WS2/F0): These exist to support DistanceEngine during transition.
    // UI code must not call these. Prefer keeping them engine-only.
    // ============================================================================
    // --- SSSP/Brandes stack helpers (DistanceEngine should not touch Stack directly) ---
    void ssspStackClear();
    bool ssspStackEmpty() const;
    int ssspStackTop() const;
    void ssspStackPop();
    int ssspStackSize() const;
    void ssspStackPush(int v);
    // --- SSSP nth-order neighborhood (for Power Centrality) ---
    void ssspNthOrderClear();
    //
    // LEGACY/INTERNAL: transitional storage.
    // DistanceEngine may use this via the accessors below.
    // (Later WS2/F1 may hide this field and keep only accessors.)
    // Stores the number of vertices at distance n from a given vertex, for n=0,1,2,... during SSSP traversal.
    H_f_i sizeOfNthOrderNeighborhood;
    H_f_i::const_iterator ssspNthOrderBegin() const;
    H_f_i::const_iterator ssspNthOrderEnd() const;
    int ssspNthOrderValue(qreal dist) const;
    void ssspNthOrderIncrement(int dist);
    void ssspNthOrderIncrement(qreal dist);
    // --- SSSP component size accumulator ---
    void ssspComponentReset(int value = 1);
    void ssspComponentAdd(int delta);
    int ssspComponentSize() const;
    // --- Connectivity bookkeeping ---
    void notConnectedPairsClear();
    void notConnectedPairsInsert(int from, int to);
    int notConnectedPairsSize() const;

    // --- Distance centrality cache flags ---
    // CLI/benchmark helper: allows repeated runs by clearing the computed flags only.
    // Does not modify graph structure or results.
    void resetDistanceCentralityCacheFlags();

    // LEGACY/INTERNAL (ENGINE SUPPORT): cached results written by DistanceEngine
    void setSymmetricCached(bool v);
    bool symmetricCached() const;

    void setConnectedCached(bool v);
    void setDiameterCached(int v);

    void resetDistanceAggregates(); // sets avg/sum/geodesics/diameter to 0
    void addToDistanceSum(qreal delta);
    void incGeodesicsCount();
    void setAverageDistanceCached(qreal v);

    bool graphMatrixDistanceGeodesicCreate(const bool &considerWeights = false,
                                           const bool &inverseWeights = false,
                                           const bool &dropIsolates = false);

    void graphMatrixShortestPathsCreate(const bool &considerWeights = false,
                                        const bool &inverseWeights = true,
                                        const bool &dropIsolates = false);

    int getProminenceIndexByName(const QString &prominenceIndexName);

    void prominenceDistribution(const int &index,
                                const ChartType &type,
                                const QString &distImageFileName = QString());

    void prominenceDistributionBars(const H_StrToInt &discreteClasses,
                                    const QString &name,
                                    const QString &distImageFileName);

    void prominenceDistributionArea(const H_StrToInt &discreteClasses,
                                    const QString &name,
                                    const QString &distImageFileName);

    void prominenceDistributionSpline(const H_StrToInt &discreteClasses,
                                      const QString &seriesName,
                                      const QString &distImageFileName);

    void centralityDegree(const bool &considerWeights = true,
                          const bool &dropIsolates = false);

    void centralityInformation(const bool considerWeights = false,
                               const bool inverseWeights = false);

    void centralityEigenvector(const bool &considerWeights = false,
                               const bool &inverseWeights = false,
                               const bool &dropIsolates = false);

    void centralityClosenessIR(const bool considerWeights = false,
                               const bool inverseWeights = false,
                               const bool dropIsolates = false);

    void prestigeDegree(const bool &considerWeights, const bool &dropIsolates = false);

    void prestigePageRank(const bool &dropIsolates = false);

    void prestigeProximity(const bool considerWeights = false,
                           const bool inverseWeights = false,
                           const bool dropIsolates = false);

    bool isCentralityIndexComputed(const IndexType index) const;

    /* REACHABILITY AND WALKS */

    int walksBetween(int v1, int v2, int length);

    void graphWalksMatrixCreate(const int &N = 0,
                                const int &length = 0,
                                const bool &updateProgress = false,
                                const bool &dropIsolates = false,
                                const bool &considerWeights = false,
                                const bool &inverseWeights = false,
                                const bool &symmetrize = false);

    void writeMatrixWalks(const QString &fn,
                          const int &length = 0,
                          const bool &simpler = false);

    QList<int> vertexinfluenceRange(int v1);

    QList<int> vertexinfluenceDomain(int v2);

    void writeReachabilityMatrixPlainText(const QString &fn,
                                          const bool &dropIsolates = false);

    qreal numberOfTriples(int v1);

    /* CLIQUES, CLUSTERING, TRIADS */
    void graphCliques(QSet<int> R = QSet<int>(), QSet<int> P = QSet<int>(), QSet<int> X = QSet<int>());

    void graphCliqueAdd(const QList<int> &clique);

    int graphCliquesContaining(const int &actor, const int &size = 0);

    int graphCliquesOfSize(const int &size);

    bool graphClusteringHierarchical(Matrix &STR_EQUIV,
                                     const QString &varLocation,
                                     const int &metric,
                                     const int &method,
                                     const bool &diagonal = false,
                                     const bool &diagram = false,
                                     const bool &considerWeights = true,
                                     const bool &inverseWeights = false,
                                     const bool &dropIsolates = false);

    qreal clusteringCoefficientLocal(const int &v1);

    qreal clusteringCoefficient(const bool updateProgress = false);

    bool graphTriadCensus();

    void triadType_examine_MAN_label(int, int, int, GraphVertex *, GraphVertex *, GraphVertex *);
    // --- Triad census results (read-only access for CLI / reports) ---
    const QList<int> &graphTriadTypeFreqs() const { return triadTypeFreqs; }
    bool hasCalculatedTriadCensus() const { return calculatedTriad; }

    //	void eccentr_JordanCenter();    // TODO

    /* LAYOUTS */

    void layoutRandom();

    void layoutRadialRandom(const bool &guides = true);

    void layoutEgoRadial(const int egoVertex);

    void layoutCircular(const double &x0,
                        const double &y0,
                        const double &newRadius,
                        const bool &guides = false);

    void layoutByProminenceIndex(int prominenceIndex,
                                 int layoutType,
                                 const bool &considerWeights = false,
                                 const bool &inverseWeights = false,
                                 const bool &dropIsolates = false);

    void layoutVertexSizeByIndegree();

    void layoutVertexSizeByOutdegree();

    void layoutForceDirectedSpringEmbedder(const int maxIterations);

    void layoutForceDirectedFruchtermanReingold(const int maxIterations);

    void layoutForceDirectedKamadaKawai(const int maxIterations = 500,
                                        const bool considerWeights = false,
                                        const bool inverseWeights = false,
                                        const bool dropIsolates = false,
                                        const QString &initialPositions = "current");

    qreal graphDistanceEuclidean(const QPointF &a, const QPointF &b);

    qreal graphDistanceEuclidean(const QPointF &a);

    int sign(const qreal &D);

    qreal layoutForceDirected_F_rep(const QString model,
                                    const qreal &dist,
                                    const qreal &optimalDistance);

    qreal layoutForceDirected_F_att(const QString model,
                                    const qreal &dist,
                                    const qreal &optimalDistance);

    qreal layoutForceDirected_Eades_moveNodes(const qreal &c4);

    qreal layoutForceDirected_FR_moveNodes(const qreal &temperature);

    qreal layoutForceDirected_FR_temperature(const int iteration) const;

    qreal computeOptimalDistance(const int &V);

    void compute_angles(const QPointF &Delta,
                        const qreal &dist,
                        qreal &angle1,
                        qreal &angle2,
                        qreal &degrees1,
                        qreal &degrees2);

    /* CRAWLER */
    void webCrawlTerminateThreads(QString reason);

    /**RANDOM NETWORKS*/
    void randomizeThings();

    bool randomNetErdosCreate(const int &N, const QString &model,
                              const int &m, const qreal &p,
                              const QString &mode, const bool &diag);

    bool randomNetScaleFreeCreate(const int &N, const int &power,
                                  const int &m0, const int &m,
                                  const qreal &alpha, const QString &mode);

    bool randomNetSmallWorldCreate(const int &N, const int &degree,
                                   const double &beta, const QString &mode);

    bool randomNetRingLatticeCreate(const int &N, const int &degree,
                                    const bool updateProgress = false);

    bool randomNetRegularCreate(const int &N, const int &degree,
                                const QString &mode, const bool &diag);

    bool randomNetLatticeCreate(const int &N, const int &length,
                                const int &dimension, const int &nei,
                                const QString &mode, const bool &circular);

    int factorial(int);

    // Progress cancellation query: readable by engines and sinks.
    bool progressCanceled() const;
    void resetProgressCanceled();

    /**  vpos stores the real position of each vertex inside m_graph.
     *  It starts at zero (0).
     *   We need to know the place of a vertex inside m_graph after adding
     *   or removing many vertices
     */
    //
    // LEGACY/INTERNAL: storage bookkeeping.
    // Do not use from UI/engines. (Later WS2 will likely privatize this and
    // expose intent-revealing helpers if needed.)
    H_Int vpos;

    // --------------------------------------------------------------------------
    // INTERNAL PROGRESS FACADE (WS2/F4)
    // Algorithm slices must not emit signals directly.
    // --------------------------------------------------------------------------
protected:
    void progressStatus(const QString &msg);
    void progressCreate(int max, const QString &msg);
    void progressUpdate(int value);
    void progressFinish();

    void uiProminenceDistributionSpline(const QVector<QPair<qreal, qreal>> &points,
                                        qreal min, qreal max,
                                        qreal minF, qreal maxF,
                                        const QString &seriesName,
                                        const QString &distImageFileName);
    void uiProminenceDistributionArea(const QVector<QPair<qreal, qreal>> &points,
                                      const qreal min,
                                      const qreal max,
                                      const qreal minF,
                                      const qreal maxF,
                                      const QString &name,
                                      const QString &distImageFileName);
    void uiProminenceDistributionBars(const QStringList &categories,
                                      const QVector<qreal> &frequencies,
                                      const qreal min,
                                      const qreal max,
                                      const qreal minF,
                                      const qreal maxF,
                                      const QString &name,
                                      const QString &distImageFileName);

private:
    /** private member functions */

    void edgeAdd(const int &v1,
                 const int &v2,
                 const qreal &weight,
                 const int &type,
                 const QString &label,
                 const QString &color);

    /** methods used by graphDistancesGeodesic()  */
    void dijkstra(const int &s,
                  const int &si,
                  const bool &computeCentralities = false,
                  const bool &inverseWeights = false,
                  const bool &dropIsolates = false);

    void minmax(qreal C,
                GraphVertex *v,
                qreal &max,
                qreal &min,
                int &maxNode, int &minNode);

    void resolveClasses(qreal C,
                        H_StrToInt &discreteClasses,
                        int &classes);

    void resolveClasses(qreal C,
                        H_StrToInt &discreteClasses,
                        int &classes, int name);

    void layoutRandomInMemory();

    VList m_graph; // List of pointers to the vertices. Each vertex stores all info: links, colors, etc

    Parser *file_parser; // Our file loader threaded class.

    WebCrawler *web_crawler; // Our web crawler threaded class. This will parse the downloaded HTML.

    QQueue<QUrl> *urlQueue; // A queue where the crawler will put urls for the network manager to download

    int m_crawler_max_urls;     // maximum urls we'll visit (max nodes in the resulted network)
    int m_crawler_visited_urls; // A counter of the urls visited.

    QList<QString> m_relationsList;

    QList<int> m_graphFileFormatExportSupported;

    QList<int> triadTypeFreqs; // stores triad type frequencies

    QList<int> m_verticesList;
    QList<int> m_verticesIsolatedList;

    QList<int> m_verticesSelected;

    QSet<int> m_verticesSet;

    QList<SelectedEdge> m_selectedEdges;
    QStack<GraphVisibilitySnapshot> m_visibilityHistory; // filter undo stack

    QMultiHash<int, int> influenceRanges, influenceDomains;

    QMultiHash<int, int> m_vertexPairsNotConnected;
    QHash<int, int> m_vertexPairsUnilaterallyConnected;

    QMultiMap<int, L_int> m_cliques;
    QHash<int, QSet<int>> neighboursHash;

    QList<qreal> m_clusteringLevel;
    QMap<int, V_int> m_clustersPerSequence;

    QMap<QString, V_int> m_clustersByName;
    QMap<int, V_str> m_clusterPairNamesPerSeq;

    Matrix SIGMA, DM, sumM, invAM, AM, invM, WM;
    Matrix XM, XSM, XRM, CLQM;

    stack<int> Stack;

    /** used in resolveClasses and graphDistancesGeodesic() */
    H_StrToInt discreteDPs, discreteSDCs, discreteCCs, discreteBCs, discreteSCs;
    H_StrToInt discreteIRCCs, discreteECs, discreteEccentricities;
    H_StrToInt discretePCs, discreteICs, discretePRPs, discretePPs, discreteEVCs;

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

    qreal minCLC, maxCLC, averageCLC, varianceCLC, d_factor;
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
    int m_reserveEdgesPerVertexSize;
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
    bool m_progressCanceled;
    bool m_graphIsDirected, m_graphIsSymmetric, m_graphIsWeighted, m_graphIsConnected;

    int csRecDepth;

    QString m_fileName, m_graphName, initEdgeColor, initVertexColor,
        initVertexNumberColor, initVertexLabelColor;
    QString initVertexShape, initVertexIconPath;
    QString htmlHead, htmlHeadLight, htmlEnd;

    QDateTime actualDateTime;
};

#endif
