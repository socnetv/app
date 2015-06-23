/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 1.9
 Written in Qt
 
                         mainwindow.h  -  description
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

#ifndef APP_H
#define APP_H


#include <QMainWindow>
#include <QGraphicsScene>
#include <QPrinter>

/** SocNetV specific includes*/

#include <math.h>
#include "graphicswidget.h"
#include "graph.h"
#include "filteredgesbyweightdialog.h"
#include "webcrawlerdialog.h"
#include "nodeeditdialog.h"
#include "datasetselectdialog.h"

static const QString VERSION="1.9";

/**
  * This Class is the base class. It sets up the main
  * window and provides a menubar, toolbar and statusbar.
  * For the main view, an instance of class GraphicsWidget is
  * created which creates a graphics widget.
  */

using namespace std;

class QMenu;
class QAction;
class QCheckBox;
class QProgressDialog;
class Edge;
class Node;

class QPushButton;
class QLCDNumber;
class QSlider;
class QComboBox;	
class QGroupBox;
class QTabWidget;
class QSpinBox;
class PreviewForm;

class RandErdosRenyiDialog;
class RandSmallWorldDialog;
class RandScaleFreeDialog;


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    GraphicsWidget *graphicsWidget;

    MainWindow(const QString &f);
    ~MainWindow();

    void initActions();
    void initMenuBar();
    void initToolBar();
    void initToolBox();
    void initStatusBar();
    void initNet();
    void initView();

    void setLastPath(QString filePath);
    QString getLastPath();
    void createFortuneCookies();
    void createTips();

    bool showLabels();
    bool showNumbersInsideNodes();
    bool showNumbers();

    // Main network file loader methods
    bool previewNetworkFile(QString , int );
    bool loadNetworkFile ( const QString, const QString, const int );

    int activeEdges();
    int activeNodes();

    void openContextMenu(const QPointF & mPos);

    void changeAllNodesSize(int size);

    QString initNodeColor;
    int clickedJimNumber; //it is public because we need to be visible from activegraph.

    void createProgressBar();
    void destroyProgressBar();

public slots:
    //FILE MENU
    void slotCreateNew();
    void slotChooseFile();
    void slotImportGraphML();
    void slotImportPajek();
    void slotImportSM();
    void slotImportDot();
    void slotImportGML();
    void slotImportDL();
    void slotImportEdgeList();
    void slotImportTwoModeSM();
    void slotFileSave();
    void slotFileSaveAs();
    void slotFileClose();
    void slotPrintView();
    bool slotExportBMP();
    bool slotExportPNG();
    bool slotExportPDF();
    void slotExportPajek();
    void slotExportSM();
    bool slotExportDL();
    bool slotExportGW();
    bool slotExportList();
    //NETWORK MENU
    void slotOpenTextEditor();
    void slotViewNetworkFile();

    void findCodecs();
    void userCodec(const QString, const QString, const int );

    void slotViewAdjacencyMatrix();
    void slotShowDataSetSelectDialog();
    void slotRecreateDataSet(QString);
    void slotCreateRandomErdosRenyi();
    void createRandomNetErdos( const int N,
                               const QString model,
                               const int edges,
                               const float eprob,
                               const QString mode,
                               const bool diag) ;
    void slotCreateRegularRandomNetwork();
    void slotCreateRandomGaussian();
    void slotCreateRandomRingLattice();
    void slotCreateRandomScaleFree();

    void createScaleFreeNetwork (const int &nodes,
                                 const int &power,
                                 const int &initialNodes,
                                 const int &edgesPerStep,
                                 const float &zeroAppeal,
                                 const QString &mode);

    void slotCreateRandomSmallWorld();

    void createSmallWorldNetwork (const int &nodes,
                                   const int &degree,
                                   const float &beta,
                                   const QString &mode,
                                   const bool &diag);
    void slotShowWebCrawlerDialog();
    void slotWebCrawl(QString, int, int, bool, bool);

    void prevRelation();
    void nextRelation();
    void addRelation();
    void addRelation(QString relationName);

    //EDIT MENU
    void slotSelectAll();
    void slotSelectNone();
    void slotFindNode();
    void slotAddEdge();
    void slotRemoveNode();
    void slotNodeProperties( const QString, const int, const QString,
                             const QColor, const QString);
    void slotRemoveEdge();
    void slotChangeNodeProperties();
    void slotChangeEdgeLabel();
    void slotChangeEdgeColor();
    void slotChangeEdgeWeight();
    void slotFilterNodes();
    void slotFilterIsolateNodes(bool checked);
    void slotShowFilterEdgesDialog();
    void slotTransformNodes2Edges();
    void slotSymmetrize();

    // LAYOUT MENU
    void slotColorationStrongStructural();
    void slotColorationRegular();
    void slotLayoutRandom();
    void slotLayoutCircularRandom();
    void slotLayoutCircularByProminenceIndex();
    void slotLayoutCircularByProminenceIndex(QString);
    void slotLayoutNodeSizesByProminenceIndex(QString);
    void slotLayoutLevelByProminenceIndex();
    void slotLayoutLevelByProminenceIndex(QString);
    void slotLayoutGuides(int);


    void slotLayoutSpringEmbedder(bool);
    void slotLayoutFruchterman();
    void layoutFruchterman(int);

    void  slotLayoutNodeSizesByOutDegree(bool);
    void  slotLayoutNodeSizesByInDegree(bool);

    //STATISTICS MENU
    void askAboutWeights();
    void slotDistancesMatrix();
    void slotGeodesicsMatrix();
    void slotGraphDistance();
    void slotAverageGraphDistance();
    void slotDiameter();
    void slotEccentricity();

    void slotWalksOfGivenLength();
    void slotTotalWalks();
    void slotReachabilityMatrix();
    void slotConnectedness();

    void slotCliqueCensus();
    void slotClusteringCoefficient();
    void slotTriadCensus();
    void slotCheckSymmetry();
    void slotInvertAdjMatrix();

    void slotCentralityDegree();
    void slotCentralityCloseness();
    void slotCentralityClosenessInfluenceRange();
    void slotCentralityBetweenness();
    void slotCentralityInformation();
    void slotCentralityStress();
    void slotCentralityPower();
    void slotCentralityEccentricity();

    void slotPrestigeDegree();
    void slotPrestigePageRank();
    void slotPrestigeProximity();

    //OPTIONS MENU
    void slotDisplayNodeNumbers(bool toggle);
    void slotDisplayNodeLabels(bool toggle);
    void slotDisplayNumbersInsideNodes(bool toggle);
    void slotChangeAllNodesSize();
    void slotChangeAllNodesShape();
    void slotChangeNumbersSize();
    void slotChangeLabelsSize();
    void slotDrawEdgesThickAsWeights();
    void slotDrawEdgesBezier(bool toggle);
    void slotDisplayEdgesWeightNumbers(bool toggle);
    void slotConsiderEdgeWeights(bool);
    void slotDisplayEdges(bool toggle);
    void slotDisplayEdgesArrows(bool toggle);

    void slotBackgroundColor ();
    void slotAllNodesColor();
    void slotAllEdgesColor();
    void slotAllNumbersColor();
    void slotAllLabelsColor();

    //VIEW MENU
    void slotAntialiasing(bool );
    void slotShowProgressBar(bool toggle);
    void slotPrintDebug(bool toggle);
    void slotViewToolBar(bool toggle);
    void slotViewStatusBar(bool toggle);
    void slotBackgroundImage(bool toggle);

    //HELP MENU
    void slotTips();
    void slotHelp();
    void slotHelpAbout();
    void slotAboutQt();
    //PUBLICLY AVAILABLE SLOTS. CALLED FROM GRAPHICSVIEW
    void nodeInfoStatusBar(Node*);
    void edgeInfoStatusBar (Edge*);
    void openNodeContextMenu();
    void openEdgeContextMenu() ;
    void windowInfoStatusBar(int, int);
    void graphChanged();

    //Called by graphicswidget to update node coords in activeGraph
    void updateNodeCoords(int no, int x, int y);

    //Called when user pushes the New Node button on the MW
    void addNode();
    //Called by graphicswidget when the user middle-clicks
    void addEdge (int v1, int v2, float weight);
    //Called by graphicswidget when the user double-clicks
    void addNodeWithMouse(int, QPointF);

    //Called by Graph on saving file. int is the network type saved.
    void networkSaved(int);

    //Called by Graph to display some message to the user
    void statusMessage(const QString);
    void showMessageToUser(const QString);

    //Called from Graph when a network file is loaded.
    void fileType(int, QString , int, int, bool);

    //Called from MW, when user highlights something in the toolbox Comboboxes
    void toolBoxAnalysisGeodesicsSelectChanged(int);
    void toolBoxAnalysisConnectivitySelectChanged(int);
    void toolBoxAnalysisProminenceSelectChanged(int);
    void toolBoxAnalysisClusterabilitySelectChanged(int);
    void toolBoxLayoutByIndexButtonPressed();

    QList<QGraphicsItem *> selectedNodes();

protected:
    void resizeEvent( QResizeEvent * );
    void closeEvent( QCloseEvent* ce );


    //	void myMessageOutput(QtMsgType type, const char *msg);
signals:
    void addRelationToGraph(QString);

private:

    QGraphicsScene *scene;

    FilterEdgesByWeightDialog m_filterEdgesByWeightDialog;
    WebCrawlerDialog m_WebCrawlerDialog;

    NodeEditDialog *m_nodeEditDialog;

    RandErdosRenyiDialog *m_randErdosRenyiDialog;
    RandSmallWorldDialog *m_randSmallWorldDialog;
    RandScaleFreeDialog *m_randScaleFreeDialog;

    PreviewForm *previewForm;
    QList<QTextCodec *> codecs;
    QString userSelectedCodecName;
    DataSetSelectDialog m_datasetSelectDialog;
    Graph activeGraph;
    QPrinter *printer;
    QToolBar *toolBar;
    QComboBox *zoomCombo, *changeRelationCombo;
    QTabWidget *toolBox;

    QProgressDialog *progressDialog;

    Node *clickedJim;
    Edge *clickedEdge;

    QMenu *importSubMenu, *exportSubMenu, *editMenu, *statMenu,  *helpMenu;
    QMenu *optionsMenu, *colorOptionsMenu, *edgeOptionsMenu, *nodeOptionsMenu, *viewOptionsMenu;
    QMenu *editNodeMenu, *editEdgeMenu, *centrlMenu, *layoutMenu;
    QMenu *networkMenu, *randomNetworkMenu, *filterMenu;
    QMenu *randomLayoutMenu, *circleLayoutMenu, *levelLayoutMenu, *physicalLayoutMenu;
    QMenu *colorationMenu;
    QCheckBox *layoutEadesBx, *layoutFruchtermanBx, *layoutKamandaBx,
    *nodeSizesByOutDegreeBx,*nodeSizesByInDegreeBx,
    *layoutGuidesBx;
    QComboBox *toolBoxAnalysisGeodesicsSelect,*toolBoxAnalysisConnectivitySelect,
            *toolBoxAnalysisProminenceSelect, *toolBoxAnalysisClusterabilitySelect;
    QComboBox *toolBoxLayoutByIndexSelect, *toolBoxLayoutByIndexTypeSelect;

    QPushButton *addNodeBt, *addEdgeBt, *removeNodeBt, *removeEdgeBt,
    *toolBoxLayoutByIndexButton;

    QSpinBox *rotateSpinBox ;

    QAction *fileNew, *fileOpen, *fileSave, *fileSaveAs,*fileClose, *printNetwork,*fileQuit;
    QAction *exportBMP, *exportPNG, *exportPajek, *exportPDF, *exportDL, *exportGW, *exportSM, *exportList;
    QAction *importPajek,*importSM, *importList,  *importDot , *importDL, *importTwoModeSM;

    QAction *viewNetworkFileAct, *openTextEditorAct, *viewSociomatrixAct, *recreateDataSetAct;

    QAction *createErdosRenyiRandomNetworkAct, *createGaussianRandomNetworkAct;
    QAction *createLatticeNetworkAct, *createScaleFreeRandomNetworkAct;
    QAction *createSmallWorldRandomNetworkAct, *createRegularRandomNetworkAct;

    QAction *displayNodeNumbersAct, *displayNodeLabelsAct, *displayNumbersInsideNodesAct;
    QAction *selectNoneAct, *selectAllAct;
    QAction *findNodeAct,*addNodeAct, *addEdgeAct, *removeNodeAct, *propertiesNodeAct, *removeEdgeAct;
    QAction *changeNumbersSizeAct;
    QAction *changeLabelsSizeAct, *changeAllNodesSizeAct, *changeAllNodesShapeAct;
    QAction *changeEdgeLabelAct, *changeEdgeColorAct, *changeEdgeWeightAct;
    QAction *filterNodesAct, *filterIsolateNodesAct, *filterEdgesAct, *transformNodes2EdgesAct, *symmetrizeAct;
    QAction *changeBackColorAct, *changeAllNodesColorAct, *changeAllEdgesColorAct, *changeAllNumbersColorAct,
            *changeAllLabelsColorAct;
    QAction *drawEdgesWeightsAct, *displayEdgesWeightNumbersAct, *displayEdgesAct;
    QAction *displayEdgesArrowsAct, *drawEdgesBezier,*considerEdgeWeightsAct;
    QAction *backgroundImageAct, *viewToolBar, *viewStatusBar, *helpAboutApp, *helpAboutQt, *helpApp, *tipsApp;
    QAction *antialiasingAct;
    QAction *webCrawlerAct;

    QAction *netDensity, *symmetryAct, *graphDistanceAct, *averGraphDistanceAct,
            *distanceMatrixAct, *geodesicsMatrixAct, *diameterAct, *eccentricityAct;
    QAction *walksAct,*totalWalksAct, *reachabilityMatrixAct, *connectednessAct;
    QAction *cliquesAct, *clusteringCoefAct, *triadCensusAct, *invertAdjMatrixAct;
    QAction *cDegreeAct, *cInDegreeAct, *cClosenessAct, *cInfluenceRangeClosenessAct,
            *cBetweennessAct, *cInformationAct, *cPageRankAct, *cStressAct,
            *cPowerAct, *cEccentAct, *cProximityPrestigeAct;
    QAction *randLayoutAct, *randCircleLayoutAct, *clearGuidesAct;
    QAction *layoutCircular_DC_Act, *layoutCircular_DP_Act,
    *layoutCircular_CC_Act, *layoutCircular_SC_Act, *layoutCircular_EC_Act,
    *layoutCircular_PC_Act, *layoutCircular_BC_Act, *layoutCircular_IC_Act,
    *layoutCircular_IRCC_Act,*layoutCircular_PRP_Act, *layoutCircular_PP_Act;
    QAction *layoutLevel_DC_Act, *layoutLevel_DP_Act,
    *layoutLevel_CC_Act, *layoutLevel_SC_Act, *layoutLevel_EC_Act,
    *layoutLevel_PC_Act, *layoutLevel_BC_Act, *layoutLevel_IC_Act,
    *layoutLevel_IRCC_Act,*layoutLevel_PRP_Act, *layoutLevel_PP_Act;
    QAction *strongColorationAct, *regularColorationAct, *showProgressBarAct, *printDebugAct;
    QAction *springLayoutAct, *FRLayoutAct;
    QAction *nodeSizesByOutDegreeAct,  *nodeSizesByInDegreeAct;
    QAction *zoomInAct, *zoomOutAct ;
    QAction *nextRelationAct, *prevRelationAct, *addRelationAct;

    QString fileName, networkName, previous_fileName;
    QString dataDir, lastUsedDirPath;
    QStringList fileNameNoPath, fortuneCookie, rgbValues;
    QStringList tempFileNameNoPath, colorList, tips;
    int statusBarDuration,  minDuration, progressCounter;
    int maxNodes;
    int initNodeSize, labelDistance, numberDistance,initNumberSize, initLabelSize;
    int fortuneCookiesCounter,  tipsCounter;
    //QString VERSION;
    bool pajekFileLoaded, adjacencyFileLoaded, dotFileLoaded, graphMLFileLoaded;
    bool fileLoaded, checkSelectFileType;
    int fileFormat;
    bool networkModified;
    bool bezier,  edgeClicked, nodeClicked, markedNodesExist, showProgressBar, firstTime;
    bool considerWeights, inverseWeights, askedAboutWeights;
    QString initFileCodec;
    QString initEdgeColor, initNumberColor,  initNodeShape, initLabelColor;
    QColor initBackgroundColor;
    QPointF cursorPosGW;	//Carries the position of the cursor in graphicsWidget coordinates
    QLCDNumber  *inDegreeLCD, *outDegreeLCD , *selectedNodeLCD, *clucofLCD;
    QLCDNumber *nodesLCD, *edgesLCD, *densityLCD;

    QDateTime actualDateTime, actualDate, actualTime;
    QTime eTime;     //used  to time algorithms.


};
#endif 

