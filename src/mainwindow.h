/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 2.2
 Written in Qt
 
                         mainwindow.h  -  description
                             -------------------
    copyright            : (C) 2005-2016 by Dimitris B. Kalamaras
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
#include <QMessageBox>

/** SocNetV specific includes*/

#include <math.h>
#include "graphicswidget.h"
#include "graph.h"
#include "filteredgesbyweightdialog.h"
#include "webcrawlerdialog.h"
#include "nodeeditdialog.h"
#include "datasetselectdialog.h"

static const QString VERSION="2.2";

static const int USER_MSG_INFO=0;
static const int USER_MSG_CRITICAL=1;
static const int USER_MSG_CRITICAL_NO_NETWORK=2;
static const int USER_MSG_CRITICAL_NO_EDGES=3;
static const int USER_MSG_QUESTION=4;
static const int USER_MSG_QUESTION_CUSTOM=5;

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
class RandRegularDialog;
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
    void initView();
    void initWindowLayout();
    void initSignalSlots();
    QMap<QString, QString> initSettings();
    void saveSettings();
    void initNet();

    void setLastPath(QString filePath);
    QString getLastPath();
    void createFortuneCookies();

    int activeEdges();
    int activeNodes();
    QList<int> selectedNodes();

    void createProgressBar(const int &max=0, const QString &msg="Please wait...");
    void destroyProgressBar(int max=0);



public slots:
    //NETWORK MENU
    void slotNetworkNew();
    void slotNetworkFileChoose(QString m_fileName = QString::null,
                               int m_fileFormat = -1,
                               const bool &checkSelectFileType = true);
    void slotNetworkFileRecentUpdateActions();
    void slotNetworkAvailableTextCodecs();
    bool slotNetworkFilePreview(const QString &, const int &);
    bool slotNetworkFileLoad ( const QString, const QString, const int );
    void slotNetworkFileLoaded(int , QString fName, QString , int, int, bool);
    void slotNetworkFileLoadRecent();
    void slotNetworkSaved(const int &status);
    void slotNetworkFileView();
    void slotNetworkImportGraphML();
    void slotNetworkImportPajek();
    void slotNetworkImportSM();
    void slotNetworkImportDot();
    void slotNetworkImportGML();
    void slotNetworkImportDL();
    void slotNetworkImportEdgeList();
    void slotNetworkImportTwoModeSM();
    void slotNetworkChanged(const int &graphStatus, const bool &undirected,
                            const int &vertices, const int &edges,
                            const float &density);
    void slotNetworkSave(const int &fileFormat=-1);
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

    void slotNetworkRandomErdosRenyiDialog();
    void slotNetworkRandomErdosRenyi( const int N,
                               const QString model,
                               const int edges,
                               const float eprob,
                               const QString mode,
                               const bool diag) ;
    void slotNetworkRandomRegularDialog();
    void slotNetworkRandomRegular(const int &newNodes, const int &degree,
                                  const QString &mode, const bool &diag);

    void slotNetworkRandomGaussian();

    void slotNetworkRandomScaleFreeDialog();

    void slotNetworkRandomScaleFree(const int &newNodes,
                                 const int &power,
                                 const int &initialNodes,
                                 const int &edgesPerStep,
                                 const float &zeroAppeal,
                                 const QString &mode);

    void slotNetworkRandomSmallWorldDialog();

    void slotNetworkRandomSmallWorld  (const int &newNodes,
                                   const int &degree,
                                   const float &beta,
                                   const QString &mode,
                                   const bool &diag);

    void slotNetworkRandomRingLattice();

    void slotNetworkWebCrawlerDialog();
    void slotNetworkWebCrawler(QString, int, int, bool, bool);

    //EDIT MENU
    void slotEditRelationPrev();
    void slotEditRelationNext();
    void slotEditRelationAdd();
    void slotEditRelationAdd(QString relationName);

    void slotEditOpenContextMenu(const QPointF & mPos);
    void slotEditSelectionChanged (const int nodes, const int edges);

    void slotEditClickOnEmptySpace ();

    void slotEditNodeSelectAll();
    void slotEditNodeSelectNone();
    void slotEditNodeInfoStatusBar(const long int &number,
                                   const QPointF &p,
                                   const QString &label,
                                   const int &inDegree,
                                   const int &outDegree,
                                   const float &clc=0);
    void slotEditNodePosition(const int &nodeNumber, const int &x, const int &y);
    void slotEditNodeAdd();
    void slotEditNodeAddWithMouse(const QPointF &);
    void slotEditNodeFind();
    void slotEditNodeRemove();
    void slotEditNodeOpenContextMenu();
    void slotEditNodePropertiesDialog();
    void slotEditNodeProperties( const QString, const int, const QString,
                             const QColor, const QString);
    void slotEditNodeSelectedToClique();
    void slotEditNodeColorAll(QColor color=QColor());
    void slotEditNodeSizeAll(int newSize=0, const bool &normalized=false);
    void slotEditNodeShape(QString shape=QString::null, const int vertex = 0);
    void slotEditNodeNumberSize(int v1=0, int newSize=0, const bool prompt=true);
    void slotEditNodeNumberDistance(int v1=0, int newSize=0);
    void slotEditNodeNumbersColor(QColor color=QColor());
    void slotEditNodeLabelSize(int v1=0, int newSize=0);
    void slotEditNodeLabelsColor(QColor color=QColor());
    void slotEditNodeLabelDistance(int v1=0, int newSize=0);

    void slotEditEdgeInfoStatusBar (const int &v1,
                                    const int &v2,
                                    const float &weight,
                                    const bool &undirected);

    void slotEditEdgeOpenContextMenu() ;
    void slotEditEdgeAdd();
    void slotEditEdgeCreate (const int &source, const int &target,
                             const float &weight=1);
    void slotEditEdgeRemove();
    void slotEditEdgeLabel();
    void slotEditEdgeColor();
    void slotEditEdgeWeight();
    void slotEditEdgeColorAll(QColor color=QColor(), const int &threshold=RAND_MAX);
    void slotEditEdgeSymmetrizeAll();
    void slotEditEdgeSymmetrizeStrongTies();
    void slotEditEdgeUndirectedAll(const bool &toggle);

    void slotFilterNodes();
    void slotEditFilterNodesIsolates(bool checked);
    void slotEditFilterEdgesByWeightDialog();
    void slotTransformNodes2Edges();
    void slotEditFilterEdgesUnilateral(bool checked);



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
    void slotLayoutGuides(const bool &toggle);
    void slotLayoutSpringEmbedder();
    void slotLayoutFruchterman();
    void slotLayoutKamadaKawai();
    void slotLayoutNodeSizesByOutDegree(bool);
    void slotLayoutNodeSizesByInDegree(bool);

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
    void slotOpenSettingsDialog();
    void slotOptionsNodeNumbersVisibility(bool toggle);
    void slotOptionsNodeNumbersInside(bool toggle);
    void slotOptionsNodeLabelsVisibility(bool toggle);
    void slotOptionsEdgesVisibility(bool toggle);
    void slotOptionsEdgeLabelsVisibility(bool toggle);
    void slotOptionsEdgeWeightNumbersVisibility(bool toggle);
    void slotOptionsEdgeWeightsDuringComputation(bool);
    void slotOptionsEdgeThicknessPerWeight(bool toogle);
    void slotOptionsEdgesBezier(bool toggle);
    void slotOptionsEdgeArrowsVisibility(bool toggle);

    void slotOptionsBackgroundColor(QColor color=QColor());
    void slotOptionsBackgroundImageSelect(bool toggle);
    void slotOptionsBackgroundImage();

    void slotOptionsAntialiasing(bool );
    void slotOptionsEmbedLogoExporting(bool toggle);
    void slotOptionsProgressBarVisibility(bool toggle);
    void slotOptionsToolbarVisibility(bool toggle);
    void slotOptionsStatusBarVisibility(bool toggle);
    void slotOptionsLeftPanelVisibility(bool toggle);
    void slotOptionsRightPanelVisibility(bool toggle);
    void slotOptionsDebugMessages(bool toggle);

    //HELP MENU
    void slotHelpTips();
    void slotHelp();
    void slotHelpCheckUpdates();
    void slotHelpCreateTips();
    void slotHelpAbout();
    void slotAboutQt();
    int slotHelpMessageToUser(const int type=0,
                              const QString statusMsg=QString::null,
                              const QString text=QString::null,
                              const QString info=QString::null,
                              QMessageBox::StandardButtons buttons=QMessageBox::NoButton,
                              QMessageBox::StandardButton defBtn=QMessageBox::Ok,
                              const QString btn1=QString::null,
                              const QString btn2=QString::null
                               );



    //Called by Graph to display some message to the user
    void statusMessage(const QString);


    //Called from MW, when user highlights something in the toolbox Comboboxes
    void toolBoxAnalysisGeodesicsSelectChanged(int);
    void toolBoxAnalysisConnectivitySelectChanged(int);
    void toolBoxAnalysisProminenceSelectChanged(int);
    void toolBoxAnalysisClusterabilitySelectChanged(int);
    void toolBoxLayoutByIndexButtonPressed();
    void toolBoxLayoutForceDirectedButtonPressed();


protected:
    void resizeEvent( QResizeEvent * );
    void closeEvent( QCloseEvent* ce );


    //	void myMessageOutput(QtMsgType type, const char *msg);
signals:
    void addRelationToGraph(QString);

private:
    QMap<QString,QString> appSettings;
    QGraphicsScene *scene;

    FilterEdgesByWeightDialog m_DialogEdgeFilterByWeight;
    WebCrawlerDialog m_WebCrawlerDialog;

    NodeEditDialog *m_nodeEditDialog;

    RandErdosRenyiDialog *m_randErdosRenyiDialog;
    RandSmallWorldDialog *m_randSmallWorldDialog;
    RandScaleFreeDialog *m_randScaleFreeDialog;
    RandRegularDialog *m_randRegularDialog;
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

    Node *clickedNode;
    Edge *clickedEdge;

    QMenu *importSubMenu, *exportSubMenu, *editMenu, *statMenu,  *helpMenu;
    QMenu *optionsMenu, *colorOptionsMenu, *edgeOptionsMenu, *nodeOptionsMenu, *viewOptionsMenu;
    QMenu *editNodeMenu, *editEdgeMenu, *centrlMenu, *layoutMenu;
    QMenu *networkMenu, *randomNetworkMenu, *filterMenu, *recentFilesSubMenu;
    QMenu *randomLayoutMenu, *circleLayoutMenu, *levelLayoutMenu, *physicalLayoutMenu;
    QMenu *colorationMenu;
    QCheckBox  *toolBoxNodeSizesByOutDegreeBx,*toolBoxNodeSizesByInDegreeBx, *toolBoxLayoutGuidesBx;
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

    QAction *optionsNodeNumbersVisibilityAct, *optionsNodeLabelsVisibilityAct, *optionsNodeNumbersInsideAct;
    QAction *editNodeSelectNoneAct, *editNodeSelectAllAct, *editNodeSelectedToCliqueAct;
    QAction *editNodeFindAct,*editNodeAddAct, *editNodeRemoveAct;
    QAction *editNodePropertiesAct;
    QAction *editEdgeAddAct, *editEdgeRemoveAct;
    QAction *editNodeNumbersSizeAct, *editNodeLabelsSizeAct;
    QAction *editNodeSizeAllAct, *editNodeShapeAll;
    QAction *editEdgeLabelAct, *editEdgeColorAct, *editEdgeWeightAct;
    QAction *filterNodesAct, *editFilterNodesIsolatesAct, *editFilterEdgesByWeightAct;
    QAction *editFilterEdgesUnilateralAct;
    QAction *transformNodes2EdgesAct, *editEdgeSymmetrizeAllAct;
    QAction *editEdgeSymmetrizeStrongTiesAct, *editEdgeUndirectedAllAct;
    QAction *changeBackColorAct, *editNodeColorAll, *editEdgeColorAllAct,
            *editNodeNumbersColorAct,*editNodeLabelsColorAct;
    QAction *optionsEdgeThicknessPerWeightAct, *optionsEdgeWeightNumbersAct, *optionsEdgesVisibilityAct;
    QAction *optionsEdgeArrowsAct, *drawEdgesBezier,*considerEdgeWeightsAct;
    QAction *optionsEdgeLabelsAct;
    QAction *backgroundImageAct,*helpAboutApp, *helpAboutQt, *helpApp, *tipsApp;
    QAction *helpCheckUpdatesApp;
    QAction *openSettingsAct;
    QAction *webCrawlerAct;

    QAction *netDensity, *symmetryAct, *graphDistanceAct, *averGraphDistanceAct,
            *distanceMatrixAct, *geodesicsMatrixAct, *diameterAct, *eccentricityAct;
    QAction *walksAct,*totalWalksAct, *reachabilityMatrixAct, *connectednessAct;
    QAction *cliquesAct, *clusteringCoefAct, *triadCensusAct, *invertAdjMatrixAct;
    QAction *cDegreeAct, *cInDegreeAct, *cClosenessAct, *cInfluenceRangeClosenessAct,
            *cBetweennessAct, *cInformationAct, *cPageRankAct, *cStressAct,
            *cPowerAct, *cEccentAct, *cProximityPrestigeAct;
    QAction *randLayoutAct, *randCircleLayoutAct, *layoutGuidesAct;
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
    enum { MaxRecentFiles = 5 };
    QAction *recentFileActs[MaxRecentFiles];

    QString fileName, networkName, previous_fileName, progressMsg;
    QString settingsFilePath, settingsDir ;
    QStringList fileNameNoPath, fortuneCookie;
    QStringList tempFileNameNoPath, tips, recentFiles;
    int clickedNodeNumber;
    int statusBarDuration, progressCounter;
    int maxNodes;
    int fortuneCookiesCounter;
    //QString VERSION;
    bool fileLoaded;

    bool networkModified;
    bool edgeClicked, nodeClicked, markedNodesExist, showProgressBar, firstTime;
    bool considerWeights, inverseWeights, askedAboutWeights;
    float randomErdosEdgeProb;
    QString initFileCodec;
    QColor initBackgroundColor;
    QPointF cursorPosGW;	//Carries the position of the cursor in graphicsWidget coordinates
    QLabel *rightPanelEdgesLabel, *rightPanelSelectedEdgesLabel, *rightPanelNetworkTypeLabel ;
    QLabel *rightPanelClickedEdgeHeaderLabel;
    QLCDNumber *rightPanelClickedNodeInDegreeLCD, *rightPanelClickedNodeOutDegreeLCD;
    QLCDNumber *rightPanelClickedNodeLCD, *rightPanelClickedNodeClucofLCD;
    QLCDNumber *rightPanelNodesLCD, *rightPanelEdgesLCD, *rightPanelDensityLCD;
    QLCDNumber *rightPanelSelectedNodesLCD, *rightPanelSelectedEdgesLCD;
    QLCDNumber *rightPanelClickedEdgeSourceLCD, *rightPanelClickedEdgeTargetLCD;
    QLCDNumber *rightPanelClickedEdgeWeightLCD;
    QDateTime actualDateTime, actualDate, actualTime;
    QTime eTime;     //used  to time algorithms.


};
#endif 

