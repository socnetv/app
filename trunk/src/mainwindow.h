/***************************************************************************
 SocNetV: Social Networks Visualiser
 version: 0.48
 Written in Qt 4.4
 
                         mainwindow.h  -  description
                             -------------------
    copyright            : (C) 2005-2008 by Dimitris B. Kalamaras
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
#include <QDateTime>
#include <QGraphicsScene>

/** SocNetV specific includes*/
#include <graphicswidget.h>
#include <math.h>
#include <utility> //declares pair construct
#include <list>
#include <vector>
#include <QSpinBox>
#include "graph.h"





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
class CanvasView;	
class QPushButton;	
class QLCDNumber;
class QSlider;
class QComboBox;	
class QGroupBox;

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
	void initDockWidget();
	void initStatusBar();
	void initNet();
	void initView();

	int loadNetworkFile( QString);
	void fileType(int , QString , int , int ); 


	int activeLinks();
	int activeNodes();
	bool symmetricAdjacency();
	void createFortuneCookies();
	void createTips();
	void makeThingsLookRandom();

	int distance(int, int);
	
	void createNode(int i, int size, QString col, QString lab, QString lCol, QPointF p, QString shape);
	void displayRandomNode(int, int,int);

	void displayRandomLink( int from, int to, int weight, bool undirected);

	bool showLabels();
	bool showNumbers();

	void openContextMenu(const QPointF & mPos);

	void changeAllNodesSize(int size);

	QString initNodeColor;
	int clickedJimNumber; //its public because we need to be visible from activegraph.

public slots:
	//FILE MENU
	void slotCreateNew();
	void slotChooseFile();
	void slotFileSave();
	void slotFileSaveAs();
	void slotFileClose();
	void slotPrintView();
	bool slotExportBMP();
	bool slotExportPNG();
	bool slotExportPajek();
	bool slotExportSM();
	bool slotExportDL();
	bool slotExportGW();
	bool slotExportList();
	//NETWORK MENU
	void slotViewNetworkFile();
	void slotViewAdjacencyMatrix();
	void slotCreateUniformRandomNetwork();
	void slotCreateSameDegreeRandomNetwork();
	void slotCreateConnectedRandomNetwork();
	void slotCreateGaussianRandomNetwork();
	void slotCreatePhysicistLatticeNetwork();

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
	void slotFilterLinks();
	void slotTransformNodes2Links();
	void slotMakeEdgesReciprocal();

	// LAYOUT MENU
	void slotColorationStrongStructural();
	void slotColorationRegular();
	void slotLayoutRandom();
	void slotLayoutRandomCircle();
	void slotLayoutCircleCentralityOutDegree();
	void slotLayoutCircleCentralityInDegree();
	void slotLayoutCircleCentralityCloseness();
	void slotLayoutCircleCentralityBetweeness();
	void slotLayoutCircleCentralityInformational();
	void slotLayoutCircleCentralityStress();
	void slotLayoutCircleCentralityGraph();
	void slotLayoutCircleCentralityEccentr();
	void slotLayoutLevelCentralityInDegree();
	void slotLayoutLevelCentralityOutDegree();
	void slotLayoutLevelCentralityCloseness();
	void slotLayoutLevelCentralityBetweeness();
	void slotLayoutLevelCentralityInformational();
	
	void slotLayoutSpringEmbedder();
	void slotLayoutFR();
	void slotZoomIn();
	void slotZoomOut();

	void  slotLayoutNodeSizeProportionalEdges(bool);

	//STATISTICS MENU
	void slotViewDistanceMatrix();
	void slotDistance();
	void slotDiameter();
	void slotActiveNodes();
	void slotActiveLinks();
	void slotCheckSymmetry();
	void slotNetworkDensity();
	void slotCentralityOutDegree();
	void slotCentralityInDegree();
	void slotCentralityCloseness();
	void slotCentralityBetweeness();
	void slotCentralityInformational();
	void slotCentralityGraph();
	void slotCentralityStress();
	void slotCentralityEccentricity();
	//OPTIONS MENU
	void slotShowNumbers(bool toggle);
	void slotShowLabels(bool toggle);
	void slotChangeAllNodesSize();
	void slotChangeAllNodesShape();
	void slotChangeNumbersSize();
	void slotChangeLabelsSize();
	void slotLinksThickWeights();
	void slotNumbersLinksWeights(bool toggle);
	void slotToggleLinks(bool toggle);
	void slotLinksArrows(bool toggle);
	void slotLinksBezier(bool toggle);
	void slotBackgroundColor ();
	void slotAllNodesColor();
	void slotAllLinksColor();
	void slotNumbersColor();
	//VIEW MENU
	void slotAntialiasing(bool );
	void slotShowProgressBar(bool toggle);
	void slotPrintDebug(bool toggle);
	void slotViewToolBar(bool toggle);
	void slotViewStatusBar(bool toggle);
	
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
	void addNodeWithMouse(int, int, int);

	//Called by graphicswidget to update node coords in activeGraph
	void updateNodeCoords(int no, int x, int y);
	//Called by graphicswidget to determine the lastAvailable NodeNumber;
	int lastAvailableNodeNumber();
	//Called when user pushes the New Node button on the MW
	void createNode();
	

protected:
	void resizeEvent( QResizeEvent * );
	void closeEvent( QCloseEvent* ce );
	QProgressDialog *actionProgress;

//	void myMessageOutput(QtMsgType type, const char *msg);
signals:


private:
	QGraphicsScene *scene;
	Graph activeGraph;
	QPrinter *printer;	
	QMenu *importSubMenu, *exportSubMenu, *editMenu, *statMenu,  *helpMenu;
	QMenu *optionsMenu, *colorOptionsMenu, *linkOptionsMenu, *nodeOptionsMenu, *viewOptionsMenu;
	QMenu *editNodeMenu, *editLinkMenu, *centrlMenu, *layoutMenu;
	QMenu *networkMenu, *randomNetworkMenu, *filterMenu;
	QMenu *randomLayoutMenu, *circleLayoutMenu, *levelLayoutMenu, *physicalLayoutMenu;
	QMenu *colorationMenu;
	QToolBar *fileToolbar, *actionToolBar;
	QComboBox *zoomCombo;
	QGroupBox *mainGroup;
	QDockWidget *leftDock;
	QCheckBox *moveSpringEmbedderBx, *moveFruchtermanBx, *moveKamandaBx, *nodeSizeProportionalEdgesBx ;

	QSpinBox *rotateSpinBox ;
	QPushButton *addNodeBt, *addLinkBt, *removeNodeBt, *removeLinkBt;

	QAction *fileNew, *fileOpen, *fileSave, *fileSaveAs,*fileClose, *filePrint,*fileQuit;
	QAction *exportBMP, *exportPNG, *exportPajek, *exportDL, *exportGW, *exportSM, *exportList;
	QAction *viewNetworkFileAct, *viewSociomatrixAct, *createUniformRandomNetworkAct;
	QAction *createGaussianRandomNetworkAct, *createLatticeNetworkAct, *createConnectedRandomNetworkAct,*createSameDegreeRandomNetworkAct;
	QAction *showNumbersAct, *showLabelsAct;
	QAction *findNodeAct,*addNodeAct, *addLinkAct, *removeNodeAct, *removeLinkAct;
	QAction *changeNumbersSizeAct, *changeNodeLabelAct, *changeNodeColorAct, *changeNodeValueAct, *changeNodeSizeAct;
	QAction *changeLabelsSizeAct, *changeAllNodesSizeAct, *changeAllNodesShapeAct;
	QAction *changeNodeBoxAct, *changeNodeCircleAct, *changeNodeTriangleAct, *changeNodeDiamondAct, *changeNodeEllipseAct;
	QAction *changeLinkLabelAct, *changeLinkColorAct, *changeLinkWeightAct;
	QAction *filterNodesAct, *filterLinksAct, *transformNodes2LinksAct, *makeEdgesReciprocalAct;
	QAction *changeBackColorAct, *changeAllNodesColorAct, *changeAllLinksColorAct, *changeNumbersColorAct;
	QAction *drawLinksWeightsAct, *showNumbersLinksWeightsAct, *showLinksAct , *showLinksArrowsAct, *drawLinksBezier;
	QAction *viewToolBar, *viewStatusBar, *helpAboutApp, *helpAboutQt, *helpApp, *tipsApp;
	QAction *netDensity, *symmetryAct, *countNodes, *countLinks,  *distanceAct, *distanceMatrixAct, *diameterAct;
	QAction *cOutDegreeAct, *cInDegreeAct, *cClosenessAct, *cBetweenessAct, *cInformationalAct, *cGraphAct, *cStressAct, *cEccentAct;
	QAction *randLayoutAct, *randCircleLayoutAct, *circleOutDegreeLayoutAct, *circleInDegreeLayoutAct, *circleClosenessLayoutAct;
	QAction *antialiasingAct;

	QAction *circleStressLayoutAct, *circleGraphLayoutAct,*circleClearBackgrCirclesAct, *circleEccentrLayoutAct;
	QAction *circleBetweenessLayoutAct, *circleInformationalLayoutAct, *levelClosenessLayoutAct;
	QAction *levelInDegreeLayoutAct, *levelOutDegreeLayoutAct, *levelBetweenessLayoutAct, *levelInformationalLayoutAct;
	QAction *strongColorationAct, *regularColorationAct, *showProgressBarAct, *printDebugAct;
	QAction *springLayoutAct, *FRLayoutAct, *nodeSizeProportionalEdgesAct;
	QAction *zoomInAct, *zoomOutAct ;

	QString fileName, networkName;
	QStringList fileNameNoPath, fortuneCookie, rgbValues;
	QStringList tempFileNameNoPath, colorList, tips;
	int statusBarDuration,  minDuration, progressCounter;
	int aNodes; 
	int initNodeSize, labelDistance, numberDistance,initNumberSize;
	int totalLinks, fortuneCookiesCounter, preSize, tipsCounter;
	QString VERSION;
	bool pajekFileLoaded, adjacencyFileLoaded, dotFileLoaded, fileLoaded,  fileSaved;
	bool networkModified;
	bool fileContainsNodesCoords, fileContainsNodeColors, fileContainsLinksColors;
	bool bezier,  linkClicked, nodeClicked, markedNodeExists, showProgressBar, firstTime;
	QString initLinkColor, initNodeShape, initLabelColor;
	QColor initNumberColor,initBackgroundColor;
	QPointF cursorPosGW;	//Carries the position of the cursor in graphicsWidget coordinates
	QLCDNumber *nodesLCD, *edgesLCD, *inLinksLCD, *outLinksLCD, *outLinkedNodesLCD, *inLinkedNodesLCD, *reciprocalLinkedNodesLCD;

	Node *clickedJim;	
	Node *markedNode;
	Edge *clickedLink;

	QDateTime actualDateTime, actualDate, actualTime;
	QTime eTime;     //used  to time algorithms.	
};
#endif 

