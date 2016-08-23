/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 2.0
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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H


/** \file mainwindow.h

 \brief Documentation for the mainwindow file.

*/

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

static const QString VERSION="2.0";


QT_BEGIN_NAMESPACE
class QMenu;
class QAction;
class QCheckBox;
class QProgressDialog;
class Edge;
class Node;

class QPushButton;
class QToolButton;
class QLCDNumber;
class QSlider;
class QComboBox;
class QGroupBox;
class QTabWidget;
class QSpinBox;
QT_END_NAMESPACE

using namespace std;

class PreviewForm;
class RandErdosRenyiDialog;
class RandSmallWorldDialog;
class RandScaleFreeDialog;
class SettingsDialog;


/**
  \brief The base window of SocNetV contains all widgets and functionality.

    It sets up the main window and provides a menubar, toolbar and statusbar.
    For the main view, an instance of class GraphicsWidget is
    created which creates a graphics widget.
 */


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
    void initView();
    void initWindowLayout();
    void initSignalSlots();
    QMap<QString, QString> initSettings();
    void saveSettings();
    void initNet();

    void setLastPath(QString filePath);
    QString getLastPath();
    void createFortuneCookies();
    void createTips();

    bool showLabels();
    bool showNumbersInsideNodes();
    bool showNumbers();

    int activeEdges();
    int activeNodes();

    int clickedJimNumber; //it is public because we need to be visible from activegraph.

    void createProgressBar(int max=0, QString msg="Please wait...");
    void destroyProgressBar();

public slots:
    //NETWORK MENU
    void slotNetworkNew();
    void slotNetworkFileChoose();
    void slotNetworkAvailableTextCodecs();
    bool slotNetworkFilePreview(const QString , const int );
    bool slotNetworkFileLoad ( const QString, const QString, const int );
    void slotNetworkFileView();
    void slotNetworkImportGraphML();
    void slotNetworkImportPajek();
    void slotNetworkImportSM();
    void slotNetworkImportDot();
    void slotNetworkImportGML();
    void slotNetworkImportDL();
    void slotNetworkImportEdgeList();
    void slotNetworkImportTwoModeSM();
    void slotNetworkChanged();
    void slotNetworkSave();
    void slotNetworkSaveAs();
    void slotNetworkClose();
    void slotNetworkPrint();
    void slotNetworkViewSociomatrix();
    bool slotNetworkExportBMP();
    bool slotNetworkExportPNG();
    bool slotNetworkExportPDF();
    void slotNetworkExportPajek();
    void slotNetworkExportSM();
    bool slotNetworkExportDL();
    bool slotNetworkExportGW();
    bool slotNetworkExportList();
    void slotNetworkTextEditor();
    void slotNetworkDataSetSelect();
    void slotNetworkDataSetRecreate(const QString);

    void slotRandomErdosRenyiDialog();
    void slotRandomErdosRenyi( const int N,
                               const QString model,
                               const int edges,
                               const float eprob,
                               const QString mode,
                               const bool diag) ;
    void slotRandomRegularNetwork();

    void slotRandomGaussian();

    void slotRandomScaleFreeDialog();

    void slotRandomScaleFree(const int &nodes,
                                 const int &power,
                                 const int &initialNodes,
                                 const int &edgesPerStep,
                                 const float &zeroAppeal,
                                 const QString &mode);

    void slotRandomSmallWorldDialog();

    void slotRandomSmallWorld  (const int &nodes,
                                   const int &degree,
                                   const float &beta,
                                   const QString &mode,
                                   const bool &diag);

    void slotRandomRingLattice();

    void slotNetworkWebCrawlerDialog();
    void slotNetworkWebCrawler(QString, int, int, bool, bool);

    //EDIT MENU

    void slotEditRelationPrev();
    void slotEditRelationNext();
    void slotEditRelationAdd();
    void slotEditRelationAdd(QString relationName);

    void slotEditOpenContextMenu(const QPointF & mPos);

    void slotEditNodeSelectAll();
    void slotEditNodeSelectNone();

    void slotEditNodeAdd();
    void slotEditNodeAddWithMouse(int, QPointF);

    void slotEditNodeFind();
    void slotEditNodeRemove();
    void slotEditNodeOpenContextMenu();
    void slotEditNodePropertiesDialog();
    void slotEditNodeProperties( const QString, const int, const QString,
                             const QColor, const QString);
    void slotEditNodeColorAll();
    void slotEditNodeColorAll(QColor);

    void slotEditNodeSizeAll();
    void slotEditNodeSizeAllNormalized(int size);

    void slotChangeAllNodesShape();
    void slotNodeShape(const QString shape, const int vertex = 0);
    void slotChangeNumbersSize();
    void slotChangeLabelsSize();


    void slotEditEdgeAdd();
    void slotEditEdgeCreate (int v1, int v2, float weight);
    void slotEditEdgeRemove();
    void slotChangeEdgeLabel();
    void slotChangeEdgeColor();
    void slotChangeEdgeWeight();

    void slotFilterNodes();
    void slotFilterIsolateNodes(bool checked);
    void slotShowFilterEdgesDialog();
    void slotTransformNodes2Edges();
    void slotSymmetrize();
    void slotSettingsBackgroundImage();
    void slotOpenSettingsDialog();

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


    void slotLayoutSpringEmbedder();
    void slotLayoutFruchterman();


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
    void slotDrawEdgesThickAsWeights();
    void slotDrawEdgesBezier(bool toggle);
    void slotDisplayEdgesWeightNumbers(bool toggle);
    void slotConsiderEdgeWeights(bool);
    void slotDisplayEdges(bool toggle);
    void slotDisplayEdgesArrows(bool toggle);

    void slotBackgroundColor ();

    void slotAllEdgesColor();
    void slotAllNumbersColor();
    void slotAllLabelsColor();

    //VIEW MENU
    void slotAntialiasing(bool );
    void slotPrintLogo(bool toggle);
    void slotShowProgressBar(bool toggle);
    void slotShowToolBar(bool toggle);
    void slotShowStatusBar(bool toggle);
    void slotShowLeftPanel(bool toggle);
    void slotShowRightPanel(bool toggle);
    void slotBackgroundImage(bool toggle);
    void slotPrintDebug(bool toggle);

    //HELP MENU
    void slotTips();
    void slotHelp();
    void slotHelpAbout();
    void slotAboutQt();
    //PUBLICLY AVAILABLE SLOTS. CALLED FROM GRAPHICSVIEW
    void nodeInfoStatusBar(Node*);
    void edgeInfoStatusBar (Edge*);

    void openEdgeContextMenu() ;


    //Called by graphicswidget to update node coords in activeGraph
    void updateNodeCoords(int no, int x, int y);


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
    void toolBoxLayoutForceDirectedButtonPressed();

    QList<QGraphicsItem *> selectedNodes();

protected:
    void resizeEvent( QResizeEvent * );
    void closeEvent( QCloseEvent* ce );


    //	void myMessageOutput(QtMsgType type, const char *msg);
signals:
    void addRelationToGraph(QString);

private:
    QMap<QString,QString> appSettings;
    QGraphicsScene *scene;

    FilterEdgesByWeightDialog m_filterEdgesByWeightDialog;
    WebCrawlerDialog m_WebCrawlerDialog;

    NodeEditDialog *m_nodeEditDialog;

    RandErdosRenyiDialog *m_randErdosRenyiDialog;
    RandSmallWorldDialog *m_randSmallWorldDialog;
    RandScaleFreeDialog *m_randScaleFreeDialog;
    SettingsDialog *m_settingsDialog;

    PreviewForm *previewForm;
    QList<QTextCodec *> codecs;
    QString userSelectedCodecName;
    DataSetSelectDialog m_datasetSelectDialog;
    Graph activeGraph;
    QPrinter *printer;
    QToolBar *toolBar;

    QGroupBox *leftPanel, *rightPanel ;

    QComboBox *editRelationChangeCombo;

    QProgressDialog *progressDialog;

    Node *clickedJim;
    Edge *clickedEdge;

    QMenu *importSubMenu, *exportSubMenu, *editMenu, *statMenu,  *helpMenu;
    QMenu *optionsMenu, *colorOptionsMenu, *edgeOptionsMenu, *nodeOptionsMenu, *viewOptionsMenu;
    QMenu *editNodeMenu, *editEdgeMenu, *centrlMenu, *layoutMenu;
    QMenu *networkMenu, *randomNetworkMenu, *filterMenu;
    QMenu *randomLayoutMenu, *circleLayoutMenu, *levelLayoutMenu, *physicalLayoutMenu;
    QMenu *colorationMenu;
    QCheckBox  *nodeSizesByOutDegreeBx,*nodeSizesByInDegreeBx, *layoutGuidesBx;
    QComboBox *toolBoxAnalysisGeodesicsSelect,*toolBoxAnalysisConnectivitySelect,
            *toolBoxAnalysisProminenceSelect, *toolBoxAnalysisClusterabilitySelect;
    QComboBox *toolBoxLayoutByIndexSelect, *toolBoxLayoutByIndexTypeSelect;
    QComboBox *toolBoxLayoutForceDirectedSelect;

    QPushButton *editNodeAddBt, *editEdgeAddBt, *removeNodeBt, *editEdgeRemoveBt;
    QPushButton *toolBoxLayoutByIndexButton, *toolBoxLayoutForceDirectedButton;

    QAction *zoomInAct,*zoomOutAct,*editRotateRightAct,*editRotateLeftAct, *editResetSlidersAct ;
    QToolButton *zoomInBtn,*zoomOutBtn,*rotateLeftBtn,*rotateRightBtn, *resetSlidersBtn ;

    QSlider *zoomSlider, *rotateSlider;

    QAction *networkNew, *networkOpen, *networkSave, *networkSaveAs,
    *networkClose, *networkPrint,*networkQuit;
    QAction *networkExportBMP, *networkExportPNG, *networkExportPajek,
    *networkExportPDF, *networkExportDL, *networkExportGW, *networkExportSM,
    *networkExportList;
    QAction *networkImportPajek,*networkImportSM, *networkImportList,
    *networkImportDot , *networkImportDL, *networkImportTwoModeSM;
    QAction *networkViewFileAct, *openTextEditorAct, *networkViewSociomatrixAct,
    *networkDataSetSelectAct;

    QAction *createErdosRenyiRandomNetworkAct, *createGaussianRandomNetworkAct;
    QAction *createLatticeNetworkAct, *createScaleFreeRandomNetworkAct;
    QAction *createSmallWorldRandomNetworkAct, *createRegularRandomNetworkAct;

    QAction *displayNodeNumbersAct, *displayNodeLabelsAct, *displayNumbersInsideNodesAct;
    QAction *editNodeSelectNoneAct, *editNodeSelectAllAct;
    QAction *editNodeFindAct,*editNodeAddAct, *editNodeRemoveAct, *propertiesNodeAct;
    QAction *editEdgeAddAct, *editEdgeRemoveAct;
    QAction *changeNumbersSizeAct;
    QAction *changeLabelsSizeAct, *changeAllNodesSizeAct, *changeAllNodesShapeAct;
    QAction *changeEdgeLabelAct, *changeEdgeColorAct, *changeEdgeWeightAct;
    QAction *filterNodesAct, *filterIsolateNodesAct, *filterEdgesAct, *transformNodes2EdgesAct, *symmetrizeAct;
    QAction *changeBackColorAct, *editNodeColorAll, *changeAllEdgesColorAct, *changeAllNumbersColorAct,
            *changeAllLabelsColorAct;
    QAction *drawEdgesWeightsAct, *displayEdgesWeightNumbersAct, *displayEdgesAct;
    QAction *displayEdgesArrowsAct, *drawEdgesBezier,*considerEdgeWeightsAct;
    QAction *backgroundImageAct,*helpAboutApp, *helpAboutQt, *helpApp, *tipsApp;
    QAction *openSettingsAct;
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
    QAction *strongColorationAct, *regularColorationAct;
    QAction *springLayoutAct, *FRLayoutAct;
    QAction *nodeSizesByOutDegreeAct,  *nodeSizesByInDegreeAct;
    QAction *editRelationNextAct, *editRelationPreviousAct, *editRelationAddAct;

    QString fileName, networkName, previous_fileName;
    QString settingsFilePath, settingsDir ;
    QStringList fileNameNoPath, fortuneCookie, rgbValues;
    QStringList tempFileNameNoPath, tips;
    int statusBarDuration, progressCounter;
    int maxNodes;
    int labelDistance, numberDistance;
    int fortuneCookiesCounter,  tipsCounter;
    //QString VERSION;
    bool pajekFileLoaded, adjacencyFileLoaded, dotFileLoaded, graphMLFileLoaded;
    bool fileLoaded, checkSelectFileType;
    int fileFormat;
    bool networkModified;
    bool bezier,  edgeClicked, nodeClicked, markedNodesExist, showProgressBar, firstTime;
    bool considerWeights, inverseWeights, askedAboutWeights;
    QString initFileCodec;

    QColor initBackgroundColor;
    QPointF cursorPosGW;	//Carries the position of the cursor in graphicsWidget coordinates
    QLCDNumber  *inDegreeLCD, *outDegreeLCD , *selectedNodeLCD, *clucofLCD;
    QLCDNumber *nodesLCD, *edgesLCD, *densityLCD;

    QDateTime actualDateTime, actualDate, actualTime;
    QTime eTime;     //used  to time algorithms.


};
#endif 

