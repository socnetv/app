/***************************************************************************
 SocNetV: Social Networks Visualizer
 version: 1.0
 Written in Qt
 
                         mainwindow.h  -  description
                             -------------------
    copyright            : (C) 2005-2013 by Dimitris B. Kalamaras
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
#include "datasetselectdialog.h"


static const QString VERSION="1.0";

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


class MainWindow : public QMainWindow
{
	Q_OBJECT

public:		/**PUBLIC FUNCTIONS NOT VISIBLE BY OTHER WIDGETS NOR BY SLOT/LINK MECHANISM */
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

	void createFortuneCookies();
	void createTips();
	void makeThingsLookRandom();

	bool showLabels();
	bool showNumbersInsideNodes();
	bool showNumbers();
	
	// Main network file loader method
	bool loadNetworkFile ( QString, int );
	

	int activeLinks();
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
	void slotViewAdjacencyMatrix();
	void slotShowDataSetSelectDialog();
	void slotRecreateDataSet(QString);
	void slotCreateRandomNetErdos();
	void slotCreateSameDegreeRandomNetwork();
	void slotCreateGaussianRandomNetwork();
	void slotCreateRandomNetRingLattice();
	void slotCreateSmallWorldRandomNetwork();
	void slotShowWebCrawlerDialog();
	void slotWebCrawl(QString, int, int, bool);
	
	//EDIT MENU
	void slotFindNode();
	void slotAddLink();
	void slotRemoveNode();
	void slotRemoveLink();
	void slotChangeNodeLabel();
	void slotChangeNodeColor();
	void slotChangeNodeValue();
	void slotChangeNodeSize();
	void slotChangeNodeBox();
	void slotChangeNodeCircle();
	void slotChangeNodeTriangle();
	void slotChangeNodeDiamond();
	void slotChangeNodeEllipse();
	void slotChangeLinkLabel();
	void slotChangeLinkColor();
	void slotChangeLinkWeight();
	void slotFilterNodes();
	void slotFilterOrphanNodes();
	void slotShowFilterEdgesDialog();
	void slotTransformNodes2Links();
	void slotSymmetrize();

	// LAYOUT MENU
	void slotColorationStrongStructural();
	void slotColorationRegular();
	void slotLayoutRandom();
	void slotLayoutRandomCircle();
	void slotLayoutRadialCentralityOutDegree();
	void slotLayoutRadialCentralityInDegree();
	void slotLayoutRadialCentralityCloseness();
	void slotLayoutRadialCentralityBetweeness();
        void slotLayoutRadialCentralityPower();
        void slotLayoutRadialCentralityInformation();
	void slotLayoutRadialCentralityStress();
	void slotLayoutRadialCentralityGraph();
	void slotLayoutRadialCentralityEccentr();
	void slotLayoutLayeredCentralityInDegree();
	void slotLayoutLayeredCentralityOutDegree();
	void slotLayoutLayeredCentralityCloseness();
	void slotLayoutLayeredCentralityBetweeness();
        void slotLayoutLayeredCentralityInformation();
	
        void slotLayoutSpringEmbedder(bool);
	void slotLayoutFruchterman();
	void layoutFruchterman(int);

	void  slotLayoutNodeSizeProportionalOutEdges(bool);
	void  slotLayoutNodeSizeProportionalInEdges(bool);

	//STATISTICS MENU
	void slotViewDistanceMatrix();
	void slotGraphDistance();
	void slotAverageGraphDistance();
	void slotDiameter();
	void slotNumberOfCliques();
	void slotClusteringCoefficient();
	void slotTriadCensus();
	void slotCheckSymmetry();
	void slotInvertAdjMatrix();

	void slotCentralityOutDegree();
	void slotCentralityInDegree();
	void slotCentralityCloseness();
	void slotCentralityBetweeness();
	void slotCentralityInformation();
	void slotCentralityGraph();
	void slotCentralityStress();
	void slotCentralityPower();
	void slotCentralityEccentricity();
    void slotCentralityPageRank();

	//OPTIONS MENU
	void slotDisplayNodeNumbers(bool toggle);
	void slotDisplayNodeLabels(bool toggle);
	void slotDisplayNumbersInsideNodes(bool toggle);
	void slotChangeAllNodesSize();
	void slotChangeAllNodesShape();
	void slotChangeNumbersSize();
	void slotChangeLabelsSize();
	void slotDrawLinksThickAsWeights();
	void slotDrawLinksBezier(bool toggle);
	void slotDisplayLinksWeightNumbers(bool toggle);
	void slotDisplayLinks(bool toggle);
	void slotDisplayLinksArrows(bool toggle);

	void slotBackgroundColor ();
	void slotAllNodesColor();
	void slotAllLinksColor();
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
	void linkInfoStatusBar (Edge*);
	void openNodeContextMenu();	
	void openLinkContextMenu() ;
	void windowInfoStatusBar(int, int);
	void graphChanged();


	//Called by graphicswidget to update node coords in activeGraph
	void updateNodeCoords(int no, int x, int y);

	//Called when user pushes the New Node button on the MW
 	void addNode();	
	//Called by graphicswidget when the user middle-clicks
	void addLink (int v1, int v2, float weight); 
	//Called by graphicswidget when the user double-clicks
	void addNodeWithMouse(int, QPointF);
	
	//Called by Graph on saving file. int is the network type saved.
	void networkSaved(int);
	
	//Called by Graph to display some message to the user
	void statusMessage(const QString);
	
	//Called by Graph to update what the selected node is. 
	void selectedNode (const int);
	
	//Called from Graph when a network file is loaded.	
	void fileType(int, QString , int, int, bool);
	 
protected:
	void resizeEvent( QResizeEvent * );
	void closeEvent( QCloseEvent* ce );


//	void myMessageOutput(QtMsgType type, const char *msg);
signals:


private:

	
	QGraphicsScene *scene;

	FilterEdgesByWeightDialog m_filterEdgesByWeightDialog;
	WebCrawlerDialog m_WebCrawlerDialog;
	DataSetSelectDialog m_datasetSelectDialog;
	Graph activeGraph;
	QPrinter *printer;	
	QToolBar *toolBar;
	QComboBox *zoomCombo;
	QTabWidget *toolBox;

	QProgressDialog *progressDialog;


	
	Node *clickedJim;	
	Node *markedNode;
	Edge *clickedLink;

	
	QMenu *importSubMenu, *exportSubMenu, *editMenu, *statMenu,  *helpMenu;
	QMenu *optionsMenu, *colorOptionsMenu, *linkOptionsMenu, *nodeOptionsMenu, *viewOptionsMenu;
	QMenu *editNodeMenu, *editLinkMenu, *centrlMenu, *layoutMenu;
	QMenu *networkMenu, *randomNetworkMenu, *filterMenu;
	QMenu *randomLayoutMenu, *circleLayoutMenu, *levelLayoutMenu, *physicalLayoutMenu;
	QMenu *colorationMenu;
	QCheckBox *moveSpringEmbedderBx, *moveFruchtermanBx, *moveKamandaBx, *nodeSizeProportional2OutDegreeBx, *nodeSizeProportional2InDegreeBx ;

	QSpinBox *rotateSpinBox ;
	QPushButton *addNodeBt, *addLinkBt, *removeNodeBt, *removeLinkBt;

	QAction *fileNew, *fileOpen, *fileSave, *fileSaveAs,*fileClose, *printNetwork,*fileQuit;
	QAction *exportBMP, *exportPNG, *exportPajek, *exportPDF, *exportDL, *exportGW, *exportSM, *exportList;
	QAction *importPajek,*importSM, *importList,  *importDot , *importDL, *importTwoModeSM;
	
	QAction *viewNetworkFileAct, *openTextEditorAct, *viewSociomatrixAct, *recreateDataSetAct; 
	
	QAction *createUniformRandomNetworkAct, *createGaussianRandomNetworkAct, *createLatticeNetworkAct;
	QAction *createSmallWorldRandomNetworkAct, *createSameDegreeRandomNetworkAct;
	QAction *displayNodeNumbersAct, *displayNodeLabelsAct, *displayNumbersInsideNodesAct;
	QAction *findNodeAct,*addNodeAct, *addLinkAct, *removeNodeAct, *removeLinkAct;
	QAction *changeNumbersSizeAct, *changeNodeLabelAct, *changeNodeColorAct, *changeNodeValueAct, *changeNodeSizeAct;
	QAction *changeLabelsSizeAct, *changeAllNodesSizeAct, *changeAllNodesShapeAct;
	QAction *changeNodeBoxAct, *changeNodeCircleAct, *changeNodeTriangleAct, *changeNodeDiamondAct, *changeNodeEllipseAct;
	QAction *changeLinkLabelAct, *changeLinkColorAct, *changeLinkWeightAct;
	QAction *filterNodesAct, *filterOrphanNodesAct, *filterEdgesAct, *transformNodes2LinksAct, *symmetrizeAct;
	QAction *changeBackColorAct, *changeAllNodesColorAct, *changeAllLinksColorAct, *changeAllNumbersColorAct, *changeAllLabelsColorAct;
	QAction *drawLinksWeightsAct, *displayLinksWeightNumbersAct, *displayLinksAct, *displayLinksArrowsAct, *drawLinksBezier;
	QAction *backgroundImageAct, *viewToolBar, *viewStatusBar, *helpAboutApp, *helpAboutQt, *helpApp, *tipsApp;
	QAction *netDensity, *symmetryAct,   *graphDistanceAct, *averGraphDistanceAct, *distanceMatrixAct, *diameterAct;
	QAction *cliquesAct, *clusteringCoefAct, *triadCensusAct, *invertAdjMatrixAct;
    QAction *cOutDegreeAct, *cInDegreeAct, *cClosenessAct, *cBetweenessAct, *cInformationAct, *cPageRankAct, *cGraphAct, *cStressAct, *cPowerAct, *cEccentAct;
	QAction *randLayoutAct, *randCircleLayoutAct, *circleOutDegreeLayoutAct, *circleInDegreeLayoutAct, *circleClosenessLayoutAct;
	QAction *antialiasingAct;
	QAction *webCrawlerAct;

        QAction *circleStressLayoutAct, *circleGraphLayoutAct,*circleClearBackgrCirclesAct, *circleEccentrLayoutAct, *circlePowerLayoutAct;
        QAction *circleBetweenessLayoutAct, *circleInformationLayoutAct, *levelClosenessLayoutAct;
        QAction *levelInDegreeLayoutAct, *levelOutDegreeLayoutAct, *levelBetweenessLayoutAct, *levelInformationLayoutAct;
	QAction *strongColorationAct, *regularColorationAct, *showProgressBarAct, *printDebugAct;
	QAction *springLayoutAct, *FRLayoutAct, *nodeSizeProportionalOutDegreeAct,  *nodeSizeProportionalInDegreeAct;
	QAction *zoomInAct, *zoomOutAct ;

	
	QString fileName, networkName, previous_fileName;
	
	QStringList fileNameNoPath, fortuneCookie, rgbValues;
	QStringList tempFileNameNoPath, colorList, tips;
	int statusBarDuration,  minDuration, progressCounter;
	int maxNodes;
    int initNodeSize, labelDistance, numberDistance,initNumberSize, initLabelSize;
	int totalLinks, fortuneCookiesCounter,  tipsCounter;
	//QString VERSION;
	bool pajekFileLoaded, adjacencyFileLoaded, dotFileLoaded, graphMLFileLoaded, fileLoaded;
    int fileFormat;
	bool networkModified;
	bool bezier,  linkClicked, nodeClicked, markedNodeExists, showProgressBar, firstTime;
	QString initLinkColor, initNumberColor,  initNodeShape, initLabelColor;
	QColor initBackgroundColor;
	QPointF cursorPosGW;	//Carries the position of the cursor in graphicsWidget coordinates
	QLCDNumber  *inLinksLCD, *outLinksLCD , *selectedNodeLCD, *clucofLCD;
	QLCDNumber *nodesLCD, *edgesLCD, *densityLCD;

	QDateTime actualDateTime, actualDate, actualTime;
	QTime eTime;     //used  to time algorithms.	
};
#endif 

