/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 2.2
 Written in Qt
 
                         mainwindow.h  -  description
                             -------------------
    copyright         : (C) 2005-2016 by Dimitris B. Kalamaras
    blog              : http://dimitris.apeiro.gr
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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H


/** \file mainwindow.h

 \brief Documentation for the mainwindow file.

*/

#include <QMainWindow>
#include <QGraphicsScene>
#include <QPrinter>
#include <QMessageBox>
#include <math.h>

/** SocNetV specific includes*/

#include "graph.h"
#include "graphicswidget.h"
#include "dialogfilteredgesbyweight.h"
#include "webcrawlerdialog.h"
#include "dialognodeedit.h"
#include "dialogdatasetselect.h"

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


class DialogPreviewFile;
class DialogRandErdosRenyi;
class DialogRandSmallWorld;
class DialogRandScaleFree;
class DialogSimilarityPearson;
class DialogSimilarityMatches;
class DialogRandRegular;
class DialogSettings;
class TextEditor;


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
    void initApp();

    void setLastPath(QString filePath);
    QString getLastPath();
    void createFortuneCookies();

    int activeEdges();
    int activeNodes();

    void createProgressBar(const int &max=0, const QString &msg="Please wait...");
    void destroyProgressBar(int max=0);



public slots:
    //NETWORK MENU
    void slotNetworkNew();
    void slotNetworkFileChoose(QString m_fileName = QString::null,
                               int m_fileFormat = -1,
                               const bool &checkSelectFileType = true);
    void slotNetworkFileDialogFileSelected(const QString &fileName);
    void slotNetworkFileDialogFilterSelected(const QString &filter);
    void slotNetworkFileDialogRejected();
    void slotNetworkFileRecentUpdateActions();
    void slotNetworkAvailableTextCodecs();
    bool slotNetworkFilePreview(const QString &, const int &);
    void slotNetworkFileLoad ( const QString, const QString, const int );
    void slotNetworkFileLoaded(const int &type,
                               const QString &fName=QString::null,
                               const QString &netName=QString::null,
                               const int &totalNodes=0,
                               const int &totalEdges=0,
                               const QString &message=QString::null);
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
    void slotNetworkViewSociomatrixPlotText();
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
    void slotEditRelationsClear();
    void slotEditRelationAdd(QString newRelationName=QString::null,
                             const bool &changeRelation=true);
    void slotEditRelationChange(const int relIndex=RAND_MAX);
    void slotEditRelationRename(QString newName=QString::null);

    void slotEditOpenContextMenu(const QPointF & mPos);
    void slotEditSelectionChanged (const int &selNodes, const int &selEdges);

    void slotEditClickOnEmptySpace (const QPointF &p);

    void slotEditNodeSelectAll();
    void slotEditNodeSelectNone();
    void slotEditNodeInfoStatusBar(const int &number,
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
    void slotEditNodeSelectedToStar();
    void slotEditNodeSelectedToCycle();
    void slotEditNodeSelectedToLine();
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
    void slotEditFilterEdgesUnilateral(bool checked);

    void slotEditTransformNodes2Edges();


    // LAYOUT MENU
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

    void slotLayoutColorationStrongStructural();
    void slotLayoutColorationRegular();

    //ANALYSIS MENU
    void askAboutWeights();
    void slotAnalyzeMatrixDistances();
    void slotAnalyzeMatrixGeodesics();
    void slotAnalyzeDistance();
    void slotAnalyzeDistanceAverage();
    void slotAnalyzeDiameter();
    void slotAnalyzeEccentricity();

    void slotAnalyzeWalksLength();
    void slotAnalyzeWalksTotal();
    void slotAnalyzeReachabilityMatrix();
    void slotAnalyzeConnectivity();

    void slotAnalyzeClusteringHierarchical();
    void slotAnalyzeCliqueCensus();
    void slotAnalyzeClusteringCoefficient();
    void slotAnalyzeTriadCensus();

    void slotAnalyzeSymmetryCheck();
    void slotAnalyzeMatrixAdjacencyInverse();
    void slotAnalyzeMatrixDegree();
    void slotAnalyzeMatrixLaplacian();

    void slotAnalyzeCentralityDegree();
    void slotAnalyzeCentralityCloseness();
    void slotAnalyzeCentralityClosenessIR();
    void slotAnalyzeCentralityBetweenness();
    void slotAnalyzeCentralityInformation();
    void slotAnalyzeCentralityStress();
    void slotAnalyzeCentralityPower();
    void slotAnalyzeCentralityEccentricity();

    void slotAnalyzePrestigeDegree();
    void slotAnalyzePrestigePageRank();
    void slotAnalyzePrestigeProximity();

    void slotAnalyzeSimilarityMatchingDialog();
    void slotAnalyzeSimilarityMatching(const QString &matrix,
                               const QString &varLocation,
                               const int &measure,
                               const bool &diagonal);

    void slotAnalyzeSimilarityPearsonDialog();
    void slotAnalyzeSimilarityPearson(const QString &matrix,
                               const QString &varLocation,
                               const bool &diagonal=false);

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
    void slotHelpMessageToUserInfo(const QString text=QString::null);
    void slotHelpMessageToUserError(const QString text=QString::null);
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
    void signalRelationAddAndChange(const QString &relName, const bool &changeRelation=true);

private:
    QGraphicsScene *scene;
    GraphicsWidget *graphicsWidget;
    Graph activeGraph;

    QMap<QString,QString> appSettings;

    DialogFilterEdgesByWeight m_DialogEdgeFilterByWeight;
    WebCrawlerDialog m_WebCrawlerDialog;
    DialogDataSetSelect m_datasetSelectDialog;

    DialogNodeEdit *m_nodeEditDialog;
    DialogRandErdosRenyi *m_randErdosRenyiDialog;
    DialogRandSmallWorld *m_randSmallWorldDialog;
    DialogRandScaleFree *m_randScaleFreeDialog;
    DialogRandRegular *m_randRegularDialog;
    DialogSettings *m_settingsDialog;
    DialogSimilarityPearson *m_dialogSimilarityPearson;
    DialogSimilarityMatches *m_dialogSimilarityMatches;


    DialogPreviewFile *m_dialogPreviewFile;
    QList<QTextCodec *> codecs;
    QString userSelectedCodecName;



    QList<TextEditor *> m_textEditors;

    QPrinter *printer;
    QToolBar *toolBar;

    QGroupBox *leftPanel, *rightPanel ;

    QComboBox *editRelationChangeCombo;

    QProgressDialog *progressDialog;

    QMenu *importSubMenu, *exportSubMenu, *editMenu, *analysisMenu, *helpMenu;
    QMenu *optionsMenu, *colorOptionsMenu, *edgeOptionsMenu, *nodeOptionsMenu;
    QMenu *editNodeMenu, *editEdgeMenu, *centrlMenu,  *viewOptionsMenu, *layoutMenu;
    QMenu *distancesMenu, *similarityMenu, *communitiesMenu, *connectivityMenu;
    QMenu *matrixMenu;
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
    QAction *networkImportPajek, *networkImportGML, *networkImportSM, *networkImportList,
    *networkImportDot , *networkImportDL, *networkImportTwoModeSM;
    QAction *networkViewFileAct, *openTextEditorAct, *networkViewSociomatrixAct,
    *networkDataSetSelectAct, *networkViewSociomatrixPlotAct;

    QAction *createErdosRenyiRandomNetworkAct, *createGaussianRandomNetworkAct;
    QAction *createLatticeNetworkAct, *createScaleFreeRandomNetworkAct;
    QAction *createSmallWorldRandomNetworkAct, *createRegularRandomNetworkAct;

    QAction *optionsNodeNumbersVisibilityAct, *optionsNodeLabelsVisibilityAct, *optionsNodeNumbersInsideAct;
    QAction *editNodeSelectNoneAct, *editNodeSelectAllAct;
    QAction *editNodeSelectedToStarAct, *editNodeSelectedToCycleAct;
    QAction *editNodeSelectedToLineAct, *editNodeSelectedToCliqueAct;
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

    QAction *netDensity, *analyzeSymmetryAct, *graphDistanceAct, *averGraphDistanceAct,
            *distanceMatrixAct, *geodesicsMatrixAct, *diameterAct, *eccentricityAct;
    QAction *walksAct,*totalWalksAct, *reachabilityMatrixAct, *connectivityAct;
    QAction *cliquesAct, *clusteringCoefAct, *triadCensusAct, *analyzeMatrixAdjInvertAct;
    QAction *analyzeMatrixDegreeAct, *analyzeMatrixLaplacianAct;
    QAction *clusteringHierarchicalAct, *similarityPearsonAct;
    QAction *similarityExactMatchesAct;
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
    QAction *editRelationRenameAct;
    enum { MaxRecentFiles = 5 };
    QAction *recentFileActs[MaxRecentFiles];

    QString fileName, previous_fileName, fileNameNoPath, progressMsg;
    QString settingsFilePath, settingsDir ;
    QStringList fortuneCookie;
    QStringList tempFileNameNoPath, tips, recentFiles;
    int statusBarDuration, progressCounter;
    int fileType, maxNodes;
    int fortuneCookiesCounter;
    //QString VERSION;

    bool markedNodesExist, showProgressBar;
    bool considerWeights, inverseWeights, askedAboutWeights;
    float randomErdosEdgeProb;
    QString initFileCodec;

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

