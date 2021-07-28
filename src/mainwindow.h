/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 3.0-rc2
 Written in Qt
 
                         mainwindow.h  -  description
                             -------------------
    copyright         : (C) 2005-2021 by Dimitris B. Kalamaras
    blog              : http://dimitris.apeiro.gr
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
*******************************************************************************/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

/** \file mainwindow.h

 \brief Documentation for the mainwindow file.

*/

#include <QMainWindow>
#include <QPrinter>
#include <QMessageBox>
#include <QStack>
#include <QThread>
#include <QNetworkReply>

// Allows to use QT_CHARTS namespace directives (see below)
#include <QtCharts/QChartGlobal>

#include "global.h"

QT_BEGIN_NAMESPACE
class QGraphicsScene;
class QMenu;
class QAction;
class QCheckBox;
class QProgressDialog;
class QPushButton;
class QToolButton;
class QPageSize;
class QSlider;
class QComboBox;
class QGroupBox;
class QTabWidget;
class QSpinBox;
class QNetworkAccessManager;
class QSslError;
class QString;
QT_END_NAMESPACE

QT_CHARTS_BEGIN_NAMESPACE
class QAbstractSeries;
class QAbstractAxis;
QT_CHARTS_END_NAMESPACE


QT_CHARTS_USE_NAMESPACE

using namespace std;

SOCNETV_USE_NAMESPACE
class Graph;
class GraphicsWidget;
class GraphicsEdge;
class GraphicsNode;
class Chart;
class DialogPreviewFile;
class DialogWebCrawler;
class DialogDataSetSelect;
class DialogRandErdosRenyi;
class DialogRandSmallWorld;
class DialogRandScaleFree;
class DialogRandRegular;
class DialogRandLattice;
class DialogSimilarityPearson;
class DialogSimilarityMatches;
class DialogDissimilarities;
class DialogClusteringHierarchical;
class DialogExportPDF;
class DialogExportImage;
class DialogNodeFind;
class DialogNodeEdit;
class DialogFilterEdgesByWeight;
class DialogEdgeDichotomization;
class DialogSettings;
class DialogSystemInfo;
class TextEditor;

typedef QHash <QString, int> H_StrToInt;


/**
  \brief The base window of SocNetV contains all widgets and functionality.

    It sets up the main window and provides a menubar, toolbar and statusbar.
    For the main view, an instance of class GraphicsWidget is
    created which creates a graphics widget.
 */

class MainWindow : public QMainWindow
{
    Q_OBJECT

    QThread graphThread;

public:

    MainWindow(const QString &m_fileName=QString(), const bool &forceProgress=false, const bool &maximized=false, const bool &fullscreen=false, const int &debugLevel=0);
    ~MainWindow();

    void slotStyleSheetDefault(const bool checked);
    void slotStyleSheetByName(const QString &sheetFileName);

    void polishProgressDialog(QProgressDialog* dialog);

    void initGraph();
    void terminateThreads(const QString &reason);
    void initView();
    void initActions();
    void initMenuBar();
    void initToolBar();
    void initPanels();
    void initWindowLayout(const bool &maximized=false, const bool &fullscreen=false);
    void initSignalSlots();
    QMap<QString, QString> initSettings(const int &debugLevel=0, const bool &forceProgress=false);
    void saveSettings();

    void initApp();

    void initComboBoxes();

    void setLastPath(const QString &filePath);
    QString getLastPath();
    void createFortuneCookies();

    int activeEdges();
    int activeNodes();

public slots:
    //NETWORK MENU
    void slotNetworkNew();
    void slotNetworkFileChoose(QString m_fileName = QString(),
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
                               const QString &fName=QString(),
                               const QString &netName=QString(),
                               const int &totalNodes=0,
                               const int &totalEdges=0,
                               const QString &message=QString());
    void slotNetworkFileLoadRecent();
    void slotNetworkSavedStatus(const int &status);
    void slotNetworkFileView();
    void slotNetworkImportGraphML();
    void slotNetworkImportPajek();
    void slotNetworkImportAdjacency();
    void slotNetworkImportGraphviz();
    void slotNetworkImportGML();
    void slotNetworkImportUcinet();
    void slotNetworkImportEdgeList();
    void slotNetworkImportTwoModeSM();

    void slotNetworkChanged(const bool &directed,
                            const int &vertices, const int &edges,
                            const qreal &density);
    void slotNetworkSave(const int &fileFormat=-1);
    void slotNetworkSaveAs();
    void slotNetworkClose();
    void slotNetworkPrint();
    void slotNetworkViewSociomatrix();
    void slotNetworkViewSociomatrixPlotText();

    void slotNetworkExportImageDialog();
    void slotNetworkExportImage ( const QString &filename,
                                  const QByteArray &format,
                                  const int &quality,
                                  const int &compression
                                  );

    void slotNetworkExportPDFDialog();
    void slotNetworkExportPDF(QString &pdfName,
                              const QPageLayout::Orientation &orientation,
                              const int &dpi,
                              const QPrinter::PrinterMode printerMode,
                              const QPageSize &pageSize);
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
                               const qreal eprob,
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
                                    const qreal &zeroAppeal,
                                    const QString &mode);

    void slotNetworkRandomSmallWorldDialog();

    void slotNetworkRandomSmallWorld  (const int &newNodes,
                                       const int &degree,
                                       const qreal &beta,
                                       const QString &mode,
                                       const bool &diag);

    void slotNetworkRandomRingLattice();

    void slotNetworkRandomLatticeDialog();
    void slotNetworkRandomLattice(const int &newNodes,
                                  const int &length,
                                  const int &dimension,
                                  const int &nei,
                                  const QString &mode,
                                  const bool &circular);

    void slotNetworkWebCrawlerDialog();

    void slotNetworkWebCrawler(const QUrl &startUrl,
                               const QStringList &urlPatternsIncluded,
                               const QStringList &urlPatternsExcluded,
                               const QStringList &linkClasses,
                               const int &maxNodes,
                               const int &maxLinksPerPage,
                               const bool &intLinks,
                               const bool &childLinks,
                               const bool &parentLinks,
                               const bool &selfLinks,
                               const bool &extLinks,
                               const bool &extLinksCrawl,
                               const bool &socialLinks,
                               const bool &delayedRequests);

    void slotNetworkManagerRequest(const QUrl &url, const NetworkRequestType &requestType);


    //EDIT MENU
    void slotEditDragModeSelection(bool);
    void slotEditDragModeScroll(bool);

    void slotEditRelationsClear();

    void slotEditRelationAddPrompt();

    void slotEditRelationAdd(const QString &newRelationName);
    void slotEditRelationChange(const int relIndex=RAND_MAX);
    void slotEditRelationRename();

    void slotEditOpenContextMenu(const QPointF & mPos);
    void slotEditSelectionChanged (const int &selNodes, const int &selEdges);

    void slotEditNodeSelectAll();
    void slotEditNodeSelectNone();
    void slotEditNodeInfoStatusBar(const int &number,
                                   const QPointF &p,
                                   const QString &label,
                                   const int &inDegree,
                                   const int &outDegree);
    void slotEditNodePosition(const int &nodeNumber, const int &x, const int &y);
    void slotEditNodeAdd();

    void slotEditNodeFindDialog();
    void slotEditNodeFind(const QStringList &list,
                          const QString &searchType,
                          const QString &indexStr=QString());
    void slotEditNodeRemove();
    void slotEditNodeOpenContextMenu();
    void slotEditNodePropertiesDialog();
    void slotEditNodeProperties( const QString &label,
                                 const int &size,
                                 const QString &value,
                                 const QColor &color,
                                 const QString &shape,
                                 const QString &iconPath
                                 );
    void slotEditNodeSelectedToClique();
    void slotEditNodeSelectedToStar();
    void slotEditNodeSelectedToCycle();
    void slotEditNodeSelectedToLine();
    void slotEditNodeColorAll(QColor color=QColor());
    void slotEditNodeSizeAll(int newSize=0, const bool &normalized=false);
    void slotEditNodeShape(const int &vertex = 0,
                           QString shape=QString(),
                           QString nodeIconPath=QString());
    void slotEditNodeNumberSize(int v1=0, int newSize=0, const bool prompt=true);
    void slotEditNodeNumberDistance(int v1=0, int newSize=0);
    void slotEditNodeNumbersColor(const int &v1=0, QColor color=QColor());
    void slotEditNodeLabelSize(const int v1=0, int newSize=0);
    void slotEditNodeLabelsColor(QColor color=QColor());
    void slotEditNodeLabelDistance(int v1=0, int newSize=0);

    void slotEditEdgeClicked (const MyEdge &edge=MyEdge(),
                                    const bool &openMenu=false);

    void slotEditEdgeOpenContextMenu(const QString &str="") ;
    void slotEditEdgeAdd();
    void slotEditEdgeCreate (const int &source, const int &target,
                             const qreal &weight=1);
    void slotEditEdgeRemove();
    void slotEditEdgeLabel();
    void slotEditEdgeColor();
    void slotEditEdgeWeight();
    void slotEditEdgeColorAll(QColor color=QColor(), const int threshold=RAND_MAX);

    void slotEditEdgeMode(const int &mode);
    void slotEditEdgeSymmetrizeAll();
    void slotEditEdgeSymmetrizeStrongTies();
    void slotEditEdgeSymmetrizeCocitation();
    void slotEditEdgeUndirectedAll(const bool &toggle);

    void slotEditEdgeDichotomizationDialog();
    void slotEditEdgeDichotomization(const qreal threshold);

    void slotFilterNodes();
    void slotEditFilterNodesIsolates(bool checked);
    void slotEditFilterEdgesByWeightDialog();
    void slotEditFilterEdgesUnilateral(bool checked);

    void slotEditTransformNodes2Edges();


    // LAYOUT MENU
    void slotLayoutRandom();
    void slotLayoutRadialRandom();

    void slotLayoutRadialByProminenceIndex();
    void slotLayoutRadialByProminenceIndex(QString prominenceIndexName);

    void slotLayoutLevelByProminenceIndex();
    void slotLayoutLevelByProminenceIndex(QString prominenceIndexName);

    void slotLayoutNodeSizeByProminenceIndex();
    void slotLayoutNodeSizeByProminenceIndex(QString prominenceIndexName);

    void slotLayoutNodeColorByProminenceIndex();
    void slotLayoutNodeColorByProminenceIndex(QString prominenceIndexName);

    void slotLayoutSpringEmbedder();
    void slotLayoutFruchterman();
    void slotLayoutKamadaKawai();

    void slotLayoutColorationStrongStructural();
    void slotLayoutColorationRegular();

    void slotLayoutGuides(const bool &toggle);

    //ANALYSIS MENU
    void askAboutWeights(const bool userTriggered=false);

    void slotAnalyzeReciprocity();
    void slotAnalyzeSymmetryCheck();
    void slotAnalyzeMatrixAdjacencyInverse();
    void slotAnalyzeMatrixAdjacencyTranspose();
    void slotAnalyzeMatrixAdjacencyCocitation();
    void slotAnalyzeMatrixDegree();
    void slotAnalyzeMatrixLaplacian();
    void slotAnalyzeClusteringCoefficient();

    void slotAnalyzeMatrixDistances();
    void slotAnalyzeMatrixGeodesics();
    void slotAnalyzeDistance();
    void slotAnalyzeDistanceAverage();
    void slotAnalyzeDiameter();
    void slotAnalyzeEccentricity();

    void slotAnalyzeWalksLength();
    void slotAnalyzeWalksTotal();
    void slotAnalyzeReachabilityMatrix();
    void slotAnalyzeConnectedness();

    void slotAnalyzeCentralityDegree();
    void slotAnalyzeCentralityCloseness();
    void slotAnalyzeCentralityClosenessIR();
    void slotAnalyzeCentralityBetweenness();
    void slotAnalyzeCentralityInformation();
    void slotAnalyzeCentralityEigenvector();
    void slotAnalyzeCentralityStress();
    void slotAnalyzeCentralityPower();
    void slotAnalyzeCentralityEccentricity();

    void slotAnalyzePrestigeDegree();
    void slotAnalyzePrestigePageRank();
    void slotAnalyzePrestigeProximity();

    void slotAnalyzeProminenceDistributionChartUpdate(QAbstractSeries *series,
                                                      QAbstractAxis *axisX,
                                                      const qreal &min,
                                                      const qreal &max,
                                                      QAbstractAxis *axisY=Q_NULLPTR,
                                                      const qreal &minF=0,
                                                      const qreal &maxF=0
                                                       );

    void slotAnalyzeCommunitiesCliqueCensus();
    void slotAnalyzeCommunitiesTriadCensus();

    void slotAnalyzeStrEquivalenceClusteringHierarchicalDialog();
    void slotAnalyzeStrEquivalenceClusteringHierarchical(const QString &matrix,
                                                         const QString &varLocation,
                                                         const QString &metric,
                                                         const QString &method,
                                                         const bool &diagonal=false,
                                                         const bool &diagram=false);

    void slotAnalyzeStrEquivalenceDissimilaritiesDialog();
    void slotAnalyzeStrEquivalenceDissimilaritiesTieProfile(const QString &metric,
                                               const QString &varLocation,
                                               const bool &diagonal);

    void slotAnalyzeStrEquivalenceSimilarityMeasureDialog();
    void slotAnalyzeStrEquivalenceSimilarityByMeasure(const QString &matrix,
                               const QString &varLocation,
                               const QString &measure,
                               const bool &diagonal);

    void slotAnalyzeStrEquivalencePearsonDialog();
    void slotAnalyzeStrEquivalencePearson(const QString &matrix,
                               const QString &varLocation,
                               const bool &diagonal=false);



    //OPTIONS MENU
    void slotOpenSettingsDialog();
    void slotOptionsNodeNumbersVisibility(bool toggle);
    void slotOptionsNodeNumbersInside(bool toggle);
    void slotOptionsNodeLabelsVisibility(bool toggle);
    void slotOptionsEdgesVisibility(bool toggle);
    void slotOptionsEdgeOffsetFromNode(const int &offset,
                                       const int &v1=0,
                                       const int &v2=0);
    void slotOptionsEdgeLabelsVisibility(bool toggle);
    void slotOptionsEdgeWeightNumbersVisibility(bool toggle);
    void slotOptionsEdgeWeightsDuringComputation(bool);
    void slotOptionsEdgeThicknessPerWeight(bool toggle);
    void slotOptionsEdgesBezier(bool toggle);
    void slotOptionsEdgeArrowsVisibility(bool toggle);

    void slotOptionsEmbedLogoExporting(bool toggle);
    void slotOptionsProgressDialogVisibility(bool toggle);

    void slotOptionsWindowToolbarVisibility(bool toggle);
    void slotOptionsWindowStatusbarVisibility(bool toggle);
    void slotOptionsWindowLeftPanelVisibility(bool toggle);
    void slotOptionsWindowRightPanelVisibility(bool toggle);
    void slotOptionsWindowFullScreen(bool toggle);

    void slotOptionsDebugMessages(bool toggle);

    void slotOptionsBackgroundColor(QColor color=QColor());
    void slotOptionsBackgroundImageSelect(bool toggle);
    void slotOptionsBackgroundImage();

    void slotOptionsCanvasOpenGL(const bool &toggle=false);
    void slotOptionsCanvasAntialiasing(bool toggle);
    void slotOptionsCanvasAntialiasingAutoAdjust(const bool &toggle=false);
    void slotOptionsCanvasSmoothPixmapTransform(const bool &toggle=false);
    void slotOptionsCanvasSavePainterState(const bool &toggle=false);
    void slotOptionsCanvasCacheBackground(const bool &toggle=false);
    void slotOptionsCanvasEdgeHighlighting(const bool &toggle=false);

    void slotOptionsCanvasUpdateMode(const QString &mode);
    void slotOptionsCanvasIndexMethod(const QString &method);


    //HELP MENU
    void slotHelpTips();
    void slotHelp();
    void slotHelpCheckUpdateDialog();
    void slotHelpCheckUpdateParse();
    void slotHelpCreateTips();
    void slotHelpSystemInfo();
    void slotHelpAbout();
    void slotAboutQt();
    void slotHelpMessageToUserInfo(const QString text=QString());
    void slotHelpMessageToUserError(const QString text=QString());
    int slotHelpMessageToUser(const int type=0,
                              const QString statusMsg=QString(),
                              const QString text=QString(),
                              const QString info=QString(),
                              QMessageBox::StandardButtons buttons=QMessageBox::NoButton,
                              QMessageBox::StandardButton defBtn=QMessageBox::Ok,
                              const QString btn1=QString(),
                              const QString btn2=QString()
                               );


    void slotNetworkManagerSslErrors(QNetworkReply *reply, const QList<QSslError> &errors);
    void slotNetworkManagerReplyError(const QNetworkReply::NetworkError &code);

    //Called by Graph to display some message to the user
    void statusMessage(const QString);


    //Called from MW, when user highlights something in the toolbox Comboboxes
    void toolBoxNetworkAutoCreateSelectChanged(const int &selectedIndex);
    void toolBoxEditNodeSubgraphSelectChanged(const int&selectedIndex);
    void toolBoxEditEdgeTransformSelectChanged(const int&selectedIndex);
    void toolBoxAnalysisMatricesSelectChanged(const int&selectedIndex);
    void toolBoxAnalysisCohesionSelectChanged(const int&selectedIndex);
    void toolBoxAnalysisStrEquivalenceSelectChanged(const int&selectedIndex);
    void toolBoxAnalysisProminenceSelectChanged(const int&selectedIndex);
    void toolBoxAnalysisCommunitiesSelectChanged(const int&selectedIndex);
    void toolBoxLayoutByIndexApplyBtnPressed();
    void toolBoxLayoutForceDirectedApplyBtnPressed();


    void slotProgressBoxCreate(const int &max=0, const QString &msg="Please wait...");
    void slotProgressBoxDestroy(const int &max=0);

protected:
    void resizeEvent(QResizeEvent * e);
    void closeEvent( QCloseEvent* ce );
signals:
    void signalRelationAddAndChange(const QString &relName, const bool &changeRelation=true);
    void signalSetReportsDataDir(const QString &dataDir );

private:

    enum { MaxRecentFiles = 5 };

    int progressCounter;
    int fileType;
    int maxRandomlyCreatedNodes;
    int fortuneCookiesCounter;

    bool inverseWeights, askedAboutWeights;

    QString fileName, previous_fileName, fileNameNoPath, progressMsg;
    QString initFileCodec, userSelectedCodecName;
    QString settingsFilePath, settingsDir ;
    QStringList fortuneCookie;
    QStringList tempFileNameNoPath, tips;
    QStringList prominenceIndexList;
    QStringList recentFiles;
    QStringList iconPathList;
    QStringList nodeShapeList;

    QMap<QString,QString> appSettings;

    QList<QTextCodec *> codecs;

    QList<TextEditor *> m_textEditors;

    QStack<QProgressDialog *> progressDialogs;

    QPrinter *printer, *printerPDF;

    QNetworkAccessManager *networkManager;
    QGraphicsScene *scene;
    GraphicsWidget *graphicsWidget;
    Graph *activeGraph;
    Chart *miniChart;

    DialogWebCrawler *m_WebCrawlerDialog;
    DialogDataSetSelect *m_datasetSelectDialog;
    DialogExportPDF *m_dialogExportPDF;
    DialogExportImage *m_dialogExportImage;
    DialogNodeEdit *m_nodeEditDialog;
    DialogNodeFind *m_nodeFindDialog;
    DialogEdgeDichotomization *m_edgeDichotomizationDialog;
    DialogFilterEdgesByWeight *m_DialogEdgeFilterByWeight;
    DialogRandErdosRenyi *m_randErdosRenyiDialog;
    DialogRandSmallWorld *m_randSmallWorldDialog;
    DialogRandScaleFree *m_randScaleFreeDialog;
    DialogRandRegular *m_randRegularDialog;
    DialogRandLattice *m_randLatticeDialog;
    DialogSimilarityPearson *m_dialogSimilarityPearson;
    DialogSimilarityMatches *m_dialogSimilarityMatches;
    DialogDissimilarities *m_dialogdissimilarities;
    DialogClusteringHierarchical *m_dialogClusteringHierarchical;
    DialogSettings *m_settingsDialog;
    DialogSystemInfo *m_systemInfoDialog;
    DialogPreviewFile *m_dialogPreviewFile;

    QToolBar *toolBar;

    QGroupBox *leftPanel, *rightPanel ;

    QComboBox *editRelationChangeCombo;

    QMenu *importSubMenu, *exportSubMenu, *editMenu, *analysisMenu, *helpMenu;
    QMenu *optionsMenu, *colorOptionsMenu, *edgeOptionsMenu, *nodeOptionsMenu;
    QMenu *editNodeMenu, *editEdgeMenu, *centrlMenu,  *viewOptionsMenu, *layoutMenu;
    QMenu *cohesionMenu, *strEquivalenceMenu, *communitiesMenu, *connectivityMenu;
    QMenu *matrixMenu;
    QMenu *networkMenu, *randomNetworkMenu, *filterMenu, *recentFilesSubMenu;
    QMenu *randomLayoutMenu, *layoutRadialProminenceMenu, *layoutLevelProminenceMenu;
    QMenu *layoutForceDirectedMenu, *layoutNodeSizeProminenceMenu, *layoutNodeColorProminenceMenu;
    QMenu *colorationMenu;

    QComboBox *toolBoxNetworkAutoCreateSelect, *toolBoxEditNodeSubgraphSelect,
              *toolBoxEditEdgeModeSelect, *toolBoxEditEdgeTransformSelect;
    QComboBox *toolBoxAnalysisCohesionSelect, *toolBoxAnalysisStrEquivalenceSelect,
    *toolBoxAnalysisProminenceSelect, *toolBoxAnalysisCommunitiesSelect,
    *toolBoxAnalysisMatricesSelect;
    QComboBox *toolBoxLayoutByIndexSelect, *toolBoxLayoutByIndexTypeSelect;
    QComboBox *toolBoxLayoutForceDirectedSelect;

    QPushButton *toolBoxLayoutByIndexApplyButton, *toolBoxLayoutForceDirectedApplyButton;

    QAction *zoomInAct,*zoomOutAct,*editRotateRightAct,*editRotateLeftAct, *editResetSlidersAct ;
    QToolButton *zoomInBtn,*zoomOutBtn,*rotateLeftBtn,*rotateRightBtn, *resetSlidersBtn ;

    QSlider *zoomSlider, *rotateSlider;

    QAction *networkNewAct, *networkOpenAct, *networkSaveAct, *networkSaveAsAct,
    *networkCloseAct, *networkPrintAct,*networkQuitAct;
    QAction *networkExportImageAct, *networkExportPajek,
    *networkExportPDFAct, *networkExportDLAct, *networkExportGWAct, *networkExportSMAct,
    *networkExportListAct;
    QAction *networkImportPajekAct, *networkImportGMLAct, *networkImportAdjAct, *networkImportListAct,
    *networkImportGraphvizAct , *networkImportUcinetAct, *networkImportTwoModeSM;
    QAction *networkViewFileAct, *openTextEditorAct, *networkViewSociomatrixAct,
    *networkDataSetSelectAct, *networkViewSociomatrixPlotAct;

    QAction *networkRandomErdosRenyiAct;
    QAction *networkRandomGaussianAct;
    QAction *networkRandomLatticeRingAct;
    QAction *networkRandomScaleFreeAct;
    QAction *networkRandomSmallWorldAct;
    QAction *networkRandomRegularSameDegreeAct;
    QAction *networkRandomLatticeAct;
    QAction *networkWebCrawlerAct;

    QAction *editDragModeScrollAct, *editDragModeSelectAct;
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
    QAction *editEdgeDichotomizeAct;
    QAction *editNodeColorAll, *editEdgeColorAllAct,
            *editNodeNumbersColorAct,*editNodeLabelsColorAct,
            *editEdgesCocitationAct;

    QAction *optionsNodeNumbersVisibilityAct, *optionsNodeLabelsVisibilityAct, *optionsNodeNumbersInsideAct;
    QAction *optionsEdgeThicknessPerWeightAct, *optionsEdgeWeightNumbersAct;
    QAction *optionsEdgesVisibilityAct;
    QAction *optionsEdgeArrowsAct, *drawEdgesBezier, *optionsEdgeWeightConsiderAct;
    QAction *optionsEdgeLabelsAct;
    QAction *backgroundImageAct,*changeBackColorAct;
    QAction *fullScreenModeAct;
    QAction *openSettingsAct;

    QAction *helpAboutApp, *helpAboutQt, *helpApp, *tipsApp;
    QAction *helpSystemInfoAct, *helpCheckUpdatesApp;

    QAction *netDensity, *analyzeGraphReciprocityAct, *analyzeGraphSymmetryAct;
    QAction *analyzeGraphDistanceAct, *averGraphDistanceAct;
    QAction *analyzeMatrixDistancesGeodesicAct, *analyzeMatrixGeodesicsAct;
    QAction *analyzeGraphDiameterAct, *analyzeGraphEccentricityAct;
    QAction *analyzeStrEquivalenceTieProfileDissimilaritiesAct;
    QAction *analyzeGraphWalksAct,*analyzeGraphWalksTotalAct, *analyzeMatrixReachabilityAct, *analyzeGraphConnectednessAct;
    QAction *analyzeCommunitiesCliquesAct, *clusteringCoefAct, *analyzeCommunitiesTriadCensusAct;
    QAction *analyzeMatrixAdjTransposeAct, *analyzeMatrixAdjInvertAct;
    QAction *analyzeMatrixAdjCocitationAct;
    QAction *analyzeMatrixDegreeAct, *analyzeMatrixLaplacianAct;
    QAction *analyzeStrEquivalenceClusteringHierarchicalAct, *analyzeStrEquivalencePearsonAct;
    QAction *analyzeStrEquivalenceMatchesAct;
    QAction *cDegreeAct, *cInDegreeAct, *cClosenessAct, *cInfluenceRangeClosenessAct,
            *cBetweennessAct, *cInformationAct, *cEigenvectorAct, *cPageRankAct,
            *cStressAct, *cPowerAct, *cEccentAct, *cProximityPrestigeAct;
    QAction *layoutRandomAct, *layoutRandomRadialAct, *layoutGuidesAct;
    QAction *layoutRadialProminence_DC_Act, *layoutRadialProminence_DP_Act,
    *layoutRadialProminence_CC_Act, *layoutRadialProminence_SC_Act, *layoutRadialProminence_EC_Act,
    *layoutRadialProminence_PC_Act, *layoutRadialProminence_BC_Act, *layoutRadialProminence_IC_Act,
    *layoutRadialProminence_EVC_Act,
    *layoutRadialProminence_IRCC_Act,*layoutRadialProminence_PRP_Act, *layoutRadialProminence_PP_Act;
    QAction *layoutLevelProminence_DC_Act, *layoutLevelProminence_DP_Act,
    *layoutLevelProminence_CC_Act, *layoutLevelProminence_SC_Act, *layoutLevelProminence_EC_Act,
    *layoutLevelProminence_PC_Act, *layoutLevelProminence_BC_Act, *layoutLevelProminence_IC_Act,
    *layoutLevelProminence_EVC_Act,
    *layoutLevelProminence_IRCC_Act,*layoutLevelProminence_PRP_Act, *layoutLevelProminence_PP_Act;
    QAction *layoutNodeSizeProminence_DC_Act, *layoutNodeSizeProminence_DP_Act,
    *layoutNodeSizeProminence_CC_Act, *layoutNodeSizeProminence_SC_Act, *layoutNodeSizeProminence_EC_Act,
    *layoutNodeSizeProminence_PC_Act, *layoutNodeSizeProminence_BC_Act, *layoutNodeSizeProminence_IC_Act,
    *layoutNodeSizeProminence_EVC_Act,
    *layoutNodeSizeProminence_IRCC_Act,*layoutNodeSizeProminence_PRP_Act, *layoutNodeSizeProminence_PP_Act;
    QAction *layoutNodeColorProminence_DC_Act, *layoutNodeColorProminence_DP_Act,
    *layoutNodeColorProminence_CC_Act, *layoutNodeColorProminence_SC_Act, *layoutNodeColorProminence_EC_Act,
    *layoutNodeColorProminence_PC_Act, *layoutNodeColorProminence_BC_Act, *layoutNodeColorProminence_IC_Act,
    *layoutNodeColorProminence_EVC_Act,
    *layoutNodeColorProminence_IRCC_Act,*layoutNodeColorProminence_PRP_Act, *layoutNodeColorProminence_PP_Act;

    QAction *strongColorationAct, *regularColorationAct;
    QAction *layoutFDP_Eades_Act, *layoutFDP_FR_Act;
    QAction *layoutFDP_KamadaKawai_Act;

    QAction *editRelationNextAct, *editRelationPreviousAct, *editRelationAddAct;
    QAction *editRelationRenameAct;
    QAction *recentFileActs[MaxRecentFiles];

    QLabel *rightPanelNetworkTypeLCD ;
    QLabel *rightPanelEdgesLabel;

    QLabel *rightPanelClickedNodeHeaderLabel;
    QLabel *rightPanelNodesLCD;
    QLabel *rightPanelEdgesLCD;
    QLabel *rightPanelDensityLCD;

    QLabel *rightPanelClickedNodeLCD;
    QLabel *rightPanelClickedNodeInDegreeLCD;
    QLabel *rightPanelClickedNodeOutDegreeLCD;

    QLabel *rightPanelSelectedNodesLCD;
    QLabel *rightPanelSelectedEdgesLCD;
    QLabel *rightPanelSelectedEdgesLabel;


    QLabel *rightPanelClickedEdgeNameLabel;
    QLabel *rightPanelClickedEdgeNameLCD;
    QLabel *rightPanelClickedEdgeWeightLabel;
    QLabel *rightPanelClickedEdgeWeightLCD;
    QLabel *rightPanelClickedEdgeReciprocalWeightLabel;
    QLabel *rightPanelClickedEdgeReciprocalWeightLCD;




};
#endif 

