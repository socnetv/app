/***************************************************************************
 SocNetV: Social Networks Visualizer
 version: 1.2
 Written in Qt

-                           mainwindow.cpp  -  description
                             -------------------
    copyright            : (C) 2005-2014 by Dimitris B. Kalamaras
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include <QtWidgets>
#include <QtGlobal>
#include <QtDebug>
#include <QPrintDialog>

#include <ctime> 

#include "mainwindow.h"
#include "graphicswidget.h"
#include "node.h"
#include "edge.h"
#include "nodenumber.h"
#include "nodelabel.h"
#include "edgeweight.h"
#include "htmlviewer.h"
#include "texteditor.h"
#include "filteredgesbyweightdialog.h"
#include "guide.h"
#include "vertex.h"




bool printDebug=false;


void myMessageOutput (
        QtMsgType type, const QMessageLogContext &context, const QString &msg )
{
    QByteArray localMsg = msg.toLocal8Bit();
    Q_UNUSED(context);
    if (printDebug)
        switch ( type ) {
        case QtDebugMsg:
            fprintf( stderr, "Debug: %s\n", localMsg.constData() );
            break;
        case QtWarningMsg:
            fprintf( stderr, "Warning: %s\n", localMsg.constData() );
            break;
        case QtFatalMsg:
            fprintf( stderr, "Fatal: %s\n", localMsg.constData() );
            abort();                    // deliberately core dump
        case QtCriticalMsg:
            fprintf( stderr, "Critical: %s\n", localMsg.constData() );
            abort();                    // deliberately core dump
        }
}


/** MainWindow contruction method **/
MainWindow::MainWindow(const QString & m_fileName) {

    qInstallMessageHandler( myMessageOutput );

    setWindowIcon (QIcon(":/images/socnetv.png"));

    /** inits that invoke all other construction parts **/
    initActions();  //register and construct menu Actions
    initMenuBar();  //construct menu
    initToolBar();  //build the toolbar
    initStatusBar();  //and now add the status bar.
    initToolBox(); //finally, build the toolbox
    //and fill a stringList with all X-supported color names
    colorList = QColor::colorNames();

    //set MW minimum size, before creating scene and canvas
    this->setMinimumSize(900,600);

    initView(); //create the canvas

    //Connect some signals to/from the canvas and the Graph
    connect( graphicsWidget, SIGNAL( selectedNode(Node*) ),
             this, SLOT( nodeInfoStatusBar(Node*) ) 	);

    connect( graphicsWidget, SIGNAL( selectedEdge(Edge*) ),
             this, SLOT ( linkInfoStatusBar(Edge*) )  );

    connect( graphicsWidget, SIGNAL( windowResized(int, int)),
             this, SLOT( windowInfoStatusBar(int,int)) 	);

    connect( graphicsWidget, SIGNAL( userDoubleClicked(int, QPointF) ),
             this, SLOT( addNodeWithMouse(int,QPointF) ) ) ;

    connect( graphicsWidget, SIGNAL( userMiddleClicked(int, int, float) ),
             this, SLOT( addLink(int, int, float) ) 	);

    connect( graphicsWidget, SIGNAL( openNodeMenu() ),
             this, SLOT( openNodeContextMenu() ) ) ;

    connect( graphicsWidget, SIGNAL( openEdgeMenu() ),
             this, SLOT( openLinkContextMenu() ) ) ;

    connect( graphicsWidget, SIGNAL(updateNodeCoords(int, int, int)),
             this, SLOT( updateNodeCoords(int, int, int) ) );

    connect( graphicsWidget, SIGNAL(zoomChanged(int)),
             zoomCombo, SLOT( setCurrentIndex(int)) );

    connect( &activeGraph, SIGNAL( addGuideCircle(int, int, int) ),
             graphicsWidget, SLOT(  addGuideCircle(int, int, int) ) ) ;

    connect( &activeGraph, SIGNAL( addGuideHLine(int) ),
             graphicsWidget, SLOT(  addGuideHLine(int) ) ) ;

    connect( &activeGraph, SIGNAL( moveNode(int, int, int) ),
             graphicsWidget, SLOT( moveNode(int, int, int) ) ) ;


    connect( &activeGraph,
             SIGNAL(
                 drawNode( int ,int,  QString, QString, int, QString, QString,
                           int, QPointF, QString, bool, bool, bool)
                 ),
             graphicsWidget,
             SLOT(
                 drawNode( int ,int,  QString, QString, int, QString, QString,
                           int, QPointF, QString, bool, bool, bool)
                 )
             ) ;

    connect( &activeGraph, SIGNAL( eraseEdge(int, int)),
             graphicsWidget, SLOT( eraseEdge(int, int) ) );

    connect( &activeGraph, SIGNAL( graphChanged() ),
             this, SLOT( graphChanged() ) ) ;

    connect( &activeGraph,
             SIGNAL(
                 signalFileType(int , QString , int , int, bool) ),
             this,
             SLOT( fileType(int , QString , int , int, bool) )
             ) ;

    connect( &activeGraph,
             SIGNAL(
                 drawEdge( int, int, float, bool, bool, QString, bool)
                 ),
             graphicsWidget,
             SLOT(
                 drawEdge( int, int,float, bool, bool, QString, bool)
                 )  ) ;

    connect( &activeGraph, SIGNAL( drawEdgeReciprocal(int, int) ),
             graphicsWidget, SLOT( drawEdgeReciprocal(int, int) ) );

    connect( &activeGraph, SIGNAL( statusMessage (QString) ),
             this, SLOT( statusMessage (QString) ) ) ;

    connect( &activeGraph, SIGNAL( selectedVertex(int) ),
             this, SLOT( selectedNode(int) ) ) ;

    connect( &activeGraph, SIGNAL( eraseNode(long int) ),
             graphicsWidget, SLOT(  eraseNode(long int) ) );


    //connect some signals/slots with MW widgets
    connect( addNodeBt,SIGNAL(clicked()), this, SLOT( addNode() ) );

    connect( addLinkBt,SIGNAL(clicked()), this, SLOT( slotAddLink() ) );

    connect( removeNodeBt,SIGNAL(clicked()), this, SLOT( slotRemoveNode() ) );

    connect( removeLinkBt,SIGNAL(clicked()), this, SLOT( slotRemoveLink() ) );

    connect( zoomCombo, SIGNAL(currentIndexChanged(const int &)),
             graphicsWidget, SLOT( changeZoom(const int &))  );

    connect( zoomOutAct, SIGNAL(triggered()), graphicsWidget, SLOT( zoomOut() ) );
    connect( zoomInAct, SIGNAL(triggered()), graphicsWidget, SLOT( zoomIn() ) );

    connect( rotateSpinBox, SIGNAL(valueChanged(int)), graphicsWidget, SLOT( rot(int) ) );

    connect( &m_filterEdgesByWeightDialog, SIGNAL( userChoices( float, bool) ),
             &activeGraph, SLOT( filterEdgesByWeight (float, bool) ) );

    connect( &m_WebCrawlerDialog, SIGNAL( userChoices( QString, int, int, bool)  ),
             this, SLOT(  slotWebCrawl( QString, int, int, bool) ) );

    connect( &m_datasetSelectDialog, SIGNAL( userChoices( QString) ),
             this, SLOT( slotRecreateDataSet(QString) ) );

    connect( &activeGraph, SIGNAL( setEdgeVisibility ( int, int, bool) ),
             graphicsWidget, SLOT(  setEdgeVisibility ( int, int, bool) ) );

    connect( &activeGraph, SIGNAL( setVertexVisibility(long int, bool)  ),
             graphicsWidget, SLOT(  setNodeVisibility (long int ,  bool) ) );

    connect( clearGuidesAct, SIGNAL(triggered()),
             graphicsWidget, SLOT(clearGuides()));


    //create an horizontal layout for the toolbox and the canvas.
    // This will be our MW layout.
    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(toolBox); 		//add them
    layout->addWidget(graphicsWidget);
    //create a dummy widget, for the above layout
    QWidget *widget = new QWidget;
    widget->setLayout(layout);
    //now set this as central widget of MW
    setCentralWidget(widget);

    /*
        initialise default network parameters
    */
    initNet();


    /*
     *  DEFAULTING HERE DOES NOT CHANGE BOOL VALUE
        EVERY TIME INITNET IS CALLED
    */
    bezier=false;
    firstTime=true;

    graphicsWidget->setInitNodeColor(initNodeColor);
    graphicsWidget->setInitNumberDistance(numberDistance);
    graphicsWidget->setInitLabelDistance(labelDistance);
    graphicsWidget->setInitNodeSize(initNodeSize);
    graphicsWidget->setBackgroundBrush(QBrush(initBackgroundColor)); //Qt::gray

    /** Try to load a GraphML network file on exec time*/
    if (!m_fileName.isEmpty())
    {
        fileName=m_fileName;
        fileNameNoPath=fileName.split ("/");
        loadNetworkFile(fileName, 0 );
    }

    if (firstTime) {
        createFortuneCookies();
        createTips();
    }

    graphicsWidget->setFocus();

    statusMessage( tr("Welcome to Social Networks Visualizer, Version ")+VERSION);

}



MainWindow::~MainWindow() {
    delete printer;
    delete scene;
    delete graphicsWidget;
}



/** initializes all QActions of the application */
void MainWindow::initActions(){
    printer = new QPrinter;

    /**
    File menu actions
    */
    fileNew = new QAction(QIcon(":/images/new.png"), tr("&New"),  this);
    fileNew->setShortcut(tr("Ctrl+N"));
    fileNew->setStatusTip(tr("Creates a new network"));
    fileNew->setToolTip(tr("New network (Ctrl+N)"));
    fileNew->setWhatsThis(tr("New\n\nCreates a new network"));
    connect(fileNew, SIGNAL(triggered()), this, SLOT(slotCreateNew()));

    fileOpen = new QAction(QIcon(":/images/open.png"), tr("&Open"), this);
    fileOpen->setShortcut(tr("Ctrl+O"));
    fileOpen->setToolTip(tr("Open network (Ctrl+O)"));
    fileOpen->setStatusTip(tr("Open a GraphML-formatted file of an existing network"));
    fileOpen->setWhatsThis(tr("Open\n\nOpens a file of an existing network in GraphML format"));
    connect(fileOpen, SIGNAL(triggered()), this, SLOT(slotImportGraphML()));


    importPajek = new QAction( QIcon(":/images/open.png"), tr("&Pajek"), this);
    importPajek->setStatusTip(tr("Import a Pajek-formatted file"));
    importPajek->setWhatsThis(tr("Import  Pajek \n\n Imports a network from a Pajek-formatted file"));
    connect(importPajek, SIGNAL(triggered()), this, SLOT(slotImportPajek()));


    importSM = new QAction( QIcon(":/images/open.png"), tr("&Adjacency Matrix"), this);
    importSM->setStatusTip(tr("Import an Adjacency matrix file"));
    importSM->setWhatsThis(tr("Import Sociomatrix \n\n  Imports a network from an Adjacency matrix-formatted file"));
    connect(importSM, SIGNAL(triggered()), this, SLOT(slotImportSM()));

    importDot = new QAction( QIcon(":/images/open.png"), tr("&Dot"), this);
    importDot->setStatusTip(tr("Import an dot file"));
    importDot->setWhatsThis(tr("Import GraphViz \n\n  Imports a network from an GraphViz formatted file"));
    connect(importDot, SIGNAL(triggered()), this, SLOT(slotImportDot()));


    importDL = new QAction( QIcon(":/images/open.png"), tr("&DL..."), this);
    importDL->setStatusTip(tr("Import network to a DL-formatted file"));
    importDL->setWhatsThis(tr("Import DL\n\nImport network to a DL-formatted"));
    connect(importDL, SIGNAL(triggered()), this, SLOT(slotImportDL()));


    importList = new QAction( QIcon(":/images/open.png"), tr("&List"), this);
    importList->setStatusTip(tr("Import network from a List-formatted file. "));
    importList->setWhatsThis(tr("Import List\n\nImport a network from a List-formatted file"));
    connect(importList, SIGNAL(triggered()), this, SLOT(slotImportEdgeList()));


    importTwoModeSM = new QAction( QIcon(":/images/open.png"), tr("&Two Mode Sociomatrix"), this);
    importTwoModeSM->setStatusTip(tr("Imports a two mode sociomatrix (affiliation network) file"));
    importTwoModeSM->setWhatsThis(tr("Import Sociomatrix \n\n  Imports a two mode network from a sociomatrix file. Two-mode networks are described by affiliation network matrices, where A(i,j) codes the events/organizations each actor is affiliated with."));
    connect(importTwoModeSM, SIGNAL(triggered()), this, SLOT(slotImportTwoModeSM()));


    fileSave = new QAction(QIcon(":/images/save.png"), tr("&Save"),  this);
    fileSave->setShortcut(tr("Ctrl+S"));
    fileSave->setToolTip(tr("Save network (Ctrl+S)"));
    fileSave->setStatusTip(tr("Saves the actual network to the current file"));
    fileSave->setWhatsThis(tr("Save.\n\nSaves the actual network"));
    connect(fileSave, SIGNAL(triggered()), this, SLOT(slotFileSave()));

    fileSaveAs = new QAction(QIcon(":/images/save.png"), tr("Save &As..."),  this);
    fileSaveAs->setShortcut(tr("Ctrl+Shift+S"));
    fileSaveAs->setStatusTip(tr("Saves the actual network under a new filename"));
    fileSaveAs->setWhatsThis(tr("Save As\n\nSaves the actual network under a new filename"));
    connect(fileSaveAs, SIGNAL(triggered()), this, SLOT(slotFileSaveAs()));

    exportBMP = new QAction(QIcon(":/images/save.png"), tr("&BMP..."), this);
    exportBMP->setStatusTip(tr("Export network to a BMP image"));
    exportBMP->setWhatsThis(tr("Export BMP \n\n Export network to a BMP image"));
    connect(exportBMP, SIGNAL(triggered()), this, SLOT(slotExportBMP()));

    exportPNG = new QAction( QIcon(":/images/save.png"), tr("&PNG..."), this);
    exportPNG->setStatusTip(tr("Export network to a PNG image"));
    exportPNG->setWhatsThis(tr("Export PNG \n\n Export network to a PNG image"));
    connect(exportPNG, SIGNAL(triggered()), this, SLOT(slotExportPNG()));


    exportPDF = new QAction( QIcon(":/images/save.png"), tr("&PDF..."), this);
    exportPDF->setStatusTip(tr("Export network to a PDF file"));
    exportPDF->setWhatsThis(tr("Export PDF\n\n Export network to a PDF document"));
    connect(exportPDF, SIGNAL(triggered()), this, SLOT(slotExportPDF()));

    exportSM = new QAction( QIcon(":/images/save.png"), tr("&Adjacency Matrix"), this);
    exportSM->setStatusTip(tr("Export network to an adjacency matrix file"));
    exportSM->setWhatsThis(tr("Export Sociomatrix \n\n Export network to a adjacency matrix-formatted file"));
    connect(exportSM, SIGNAL(triggered()), this, SLOT(slotExportSM()));

    exportPajek = new QAction( QIcon(":/images/save.png"), tr("&Pajek"), this);
    exportPajek->setStatusTip(tr("Export network to a Pajek-formatted file"));
    exportPajek->setWhatsThis(tr("Export Pajek \n\n Export network to a Pajek-formatted file"));
    connect(exportPajek, SIGNAL(triggered()), this, SLOT(slotExportPajek()));

    exportList = new QAction( QIcon(":/images/save.png"), tr("&List"), this);
    exportList->setStatusTip(tr("Export network to a List-formatted file. "));
    exportList->setWhatsThis(tr("Export List\n\nExport network to a List-formatted file"));
    connect(exportList, SIGNAL(triggered()), this, SLOT(slotExportList()));

    exportDL = new QAction( QIcon(":/images/save.png"), tr("&DL..."), this);
    exportDL->setStatusTip(tr("Export network to a DL-formatted file"));
    exportDL->setWhatsThis(tr("Export DL\n\nExport network to a DL-formatted"));
    connect(exportDL, SIGNAL(triggered()), this, SLOT(slotExportDL()));

    exportGW = new QAction( QIcon(":/images/save.png"), tr("&GW..."), this);
    exportGW->setStatusTip(tr("Export network to a GW-formatted file"));
    exportGW->setWhatsThis(tr("Export\n\nExport network to a GW formatted file"));
    connect(exportGW, SIGNAL(triggered()), this, SLOT(slotExportGW()));

    fileClose = new QAction( tr("&Close"), this);
    fileClose->setStatusTip(tr("Closes the actual network"));
    fileClose->setWhatsThis(tr("Close \n\nCloses the actual network"));
    connect(fileClose, SIGNAL(triggered()), this, SLOT(slotFileClose()));

    printNetwork = new QAction(QIcon(":/images/print.png"), tr("&Print"), this);
    printNetwork->setShortcut(tr("Ctrl+P"));
    printNetwork->setStatusTip(tr("Prints whatever is viewable on the canvas."));
    printNetwork->setWhatsThis(tr("Printing \n\n This function prints whatever is viewable on the canvas. \nTo print the whole network, you might want to zoom-out."));
    connect(printNetwork, SIGNAL(triggered()), this, SLOT(slotPrintView()));

    fileQuit = new QAction(QIcon(":/images/exit.png"), tr("E&xit"), this);
    fileQuit->setShortcut(tr("Ctrl+Q"));
    fileQuit->setStatusTip(tr("Quits the application"));
    fileQuit->setWhatsThis(tr("Exit\n\nQuits the application"));
    connect(fileQuit, SIGNAL(triggered()), this, SLOT(close()));


    openTextEditorAct = new QAction(QIcon(""), tr("Open Text Editor"),this);
    openTextEditorAct ->setShortcut(tr("Shift+F5"));
    openTextEditorAct->setStatusTip(tr("Opens the SocNetV text editor. You can copy/paste network data, save and then import them..."));
    openTextEditorAct->setWhatsThis(tr("Open Text Editor\n\nOpens the SocNetV text editor where you can copy paste network data, of any supported format, and save to a file. Then you can import that file to SocNetV..."));
    connect(openTextEditorAct, SIGNAL(triggered()), this, SLOT(slotOpenTextEditor()));


    viewNetworkFileAct = new QAction(QIcon(":/images/net2.png"), tr("View Loaded File"),this);
    viewNetworkFileAct ->setShortcut(tr("F5"));
    viewNetworkFileAct->setStatusTip(tr("Displays the loaded network file"));
    viewNetworkFileAct->setWhatsThis(tr("View Loaded File\n\nDisplays the file of the loaded network"));
    connect(viewNetworkFileAct, SIGNAL(triggered()), this, SLOT(slotViewNetworkFile()));

    viewSociomatrixAct = new QAction(QIcon(":/images/sm.png"), tr("View Adjacency Matrix"),  this);
    viewSociomatrixAct ->setShortcut(tr("F6"));
    viewSociomatrixAct->setStatusTip(tr("Displays the adjacency matrix of the active network. See manual or online help for more..."));
    viewSociomatrixAct->setWhatsThis(tr("View Adjacency Matrix\n\nDisplays the adjacency matrix of the active network. \n\n The adjacency matrix of a network is a matrix where each element a(i,j) is equal to the weight of the link from node i to node j. If the nodes are not connected, then a(i,j)=0. "));
    connect(viewSociomatrixAct, SIGNAL(triggered()), this, SLOT(slotViewAdjacencyMatrix()));


    recreateDataSetAct = new QAction(QIcon(":/images/sm.png"), tr("Create Known Data Sets"),  this);
    recreateDataSetAct ->setShortcut(tr("F7"));
    recreateDataSetAct->setStatusTip(tr("Recreates a variety of known data sets."));
    recreateDataSetAct->setWhatsThis(tr("Known Data Sets\n\nRecreates some of the most widely used data sets in network analysis studies"));
    connect(recreateDataSetAct, SIGNAL(triggered()), this, SLOT(slotShowDataSetSelectDialog()));



    createUniformRandomNetworkAct = new QAction(QIcon(":/images/erdos.png"), tr("Erdos-Renyi G(n,p)"),  this);
    createUniformRandomNetworkAct ->setShortcut(tr("Shift+U"));
    createUniformRandomNetworkAct->setStatusTip(tr("Creates a random network where each edge is included with a given probability"));
    createUniformRandomNetworkAct->setWhatsThis(tr("Uniform \n\nCreates a random network of G(n, p) model by connecting nodes randomly. Each edge is included in the graph with equal probability p, independently of the other edges"));
    connect(createUniformRandomNetworkAct, SIGNAL(triggered()), this, SLOT(slotCreateRandomNetErdos()));

    createLatticeNetworkAct = new QAction( QIcon(":/images/net1.png"), tr("Ring Lattice"), this);
    createLatticeNetworkAct ->setShortcut(tr("Shift+L"));
    createLatticeNetworkAct->setStatusTip(tr("Creates a ring lattice random network"));
    createLatticeNetworkAct->setWhatsThis(tr("Ring Lattice \n\nA ring lattice or a physicist's lattice is a graph with N nodes each connected to K neighbors, K / 2 on each side."));
    connect(createLatticeNetworkAct, SIGNAL(triggered()), this, SLOT(slotCreateRandomNetRingLattice()));

    createSameDegreeRandomNetworkAct = new QAction(QIcon(":/images/net.png"), tr("Same Degree"), this);
    createSameDegreeRandomNetworkAct->setStatusTip(tr("Creates a random network where all nodes have the same degree."));
    createSameDegreeRandomNetworkAct->setWhatsThis(tr("Same Degree \n\nCreates a random network where all nodes have the same degree "));
    connect(createSameDegreeRandomNetworkAct, SIGNAL(triggered()), this, SLOT(slotCreateSameDegreeRandomNetwork()));

    createGaussianRandomNetworkAct = new QAction(tr("Gaussian"),	this);
    createGaussianRandomNetworkAct->setStatusTip(tr("Creates a Gaussian distributed random network"));
    createGaussianRandomNetworkAct->setWhatsThis(tr("Gaussian \n\nCreates a random network of Gaussian distribution"));
    connect(createGaussianRandomNetworkAct, SIGNAL(triggered()), this, SLOT(slotCreateGaussianRandomNetwork()));

    createSmallWorldRandomNetworkAct = new QAction(QIcon(":/images/sw.png"), tr("Small World"),	this);
    createSmallWorldRandomNetworkAct->setShortcut(tr("Shift+W"));
    createSmallWorldRandomNetworkAct->setStatusTip(tr("Creates a random network with small world properties"));
    createSmallWorldRandomNetworkAct->setWhatsThis(tr("Small World \n\nA Small World, according to the Watts and Strogatz model, is a random network with short average path lengths and high clustering coefficient."));
    connect(createSmallWorldRandomNetworkAct, SIGNAL(triggered()), this, SLOT(slotCreateSmallWorldRandomNetwork()));




    webCrawlerAct = new QAction(QIcon(":/images/webcrawler.png"), tr("Web Crawler"),	this);
    webCrawlerAct->setShortcut(tr("Shift+C"));
    webCrawlerAct->setStatusTip(tr("Creates a network from all links found in a given website"));
    webCrawlerAct->setWhatsThis(tr("Web Crawler \n\nA Web crawler is a built-in bot, which starts with a given URL (website or webpage) to visit. As the algorithm crawls this webpage, it identifies all the links in the page and adds them to a list of URLs (called frontier). Then, all the URLs from the frontier are recursively visited. You must provide maximum recursion level (how many URLs from the frontier will be visited) and maximum running time, along with the initial web address..."));
    connect(webCrawlerAct, SIGNAL(triggered()), this, SLOT(slotShowWebCrawlerDialog()));


    /**
    Edit menu actions
    */
    findNodeAct = new QAction(QIcon(":/images/find.png"), tr("Find Node"), this);
    findNodeAct->setShortcut(tr("Ctrl+F"));
    findNodeAct->setStatusTip(tr("Finds and highlights a node by number or label. Press Ctrl+F again to undo."));
    findNodeAct->setWhatsThis(tr("Find Node\n\nFinds a node with a given number or label and doubles its size. Ctrl+F again resizes back the node"));
    connect(findNodeAct, SIGNAL(triggered()), this, SLOT(slotFindNode()) );

    addNodeAct = new QAction(QIcon(":/images/add.png"), tr("Add Node"), this);
    addNodeAct->setShortcut(tr("Ctrl+A"));
    addNodeAct->setStatusTip(tr("Adds a node"));
    addNodeAct->setWhatsThis(tr("Add Node\n\nAdds a node to the network"));
    connect(addNodeAct, SIGNAL(triggered()), this, SLOT(addNode()));

    removeNodeAct = new QAction(QIcon(":/images/remove.png"),tr("Remove Node"), this);
    removeNodeAct ->setShortcut(tr("Ctrl+Shift+A"));
    removeNodeAct->setStatusTip(tr("Removes a node"));
    removeNodeAct->setWhatsThis(tr("Remove Node\n\nRemoves a node from the network"));
    connect(removeNodeAct, SIGNAL(triggered()), this, SLOT(slotRemoveNode()));

    changeNodeLabelAct = new QAction(QIcon(":/images/letters.png"), tr("Change Label"),	this);
    changeNodeLabelAct->setStatusTip(tr("Changes the Label of a node"));
    changeNodeLabelAct->setWhatsThis(tr("Change Label\n\nChanges the label of a node"));
    connect(changeNodeLabelAct, SIGNAL(triggered()), this, SLOT(slotChangeNodeLabel()));

    changeNodeColorAct = new QAction(QIcon(":/images/colorize.png"), tr("Change Color"), this);
    changeNodeColorAct->setStatusTip(tr("Changes the color of a node"));
    changeNodeColorAct->setWhatsThis(tr("Change Color\n\nChanges the Color of a node"));
    connect(changeNodeColorAct, SIGNAL(triggered()), this, SLOT(slotChangeNodeColor()));

    changeNodeSizeAct = new QAction(QIcon(":/images/resize.png"),tr("Change Size"), this);
    changeNodeSizeAct->setStatusTip(tr("Changes the actual size of a node"));
    changeNodeSizeAct->setWhatsThis(tr("Change Size\n\nChanges the actual size of a node"));
    connect(changeNodeSizeAct, SIGNAL(triggered()), this, SLOT(slotChangeNodeSize()));

    changeNodeValueAct = new QAction(tr("Change Value"), this);
    changeNodeValueAct->setStatusTip(tr("Changes the value of a node"));
    changeNodeValueAct->setWhatsThis(tr("Change Value\n\nChanges the value of a node"));
    connect(changeNodeValueAct, SIGNAL(triggered()), this, SLOT(slotChangeNodeValue()));

    changeAllNodesSizeAct = new QAction(QIcon(":/images/resize.png"), tr("Change all Nodes Size"),	this);
    changeAllNodesSizeAct->setStatusTip(tr("This option lets you change the size of all nodes"));
    changeAllNodesSizeAct->setWhatsThis(tr("Nodes Size\n\nThis option lets you change the size of all nodes"));
    connect(changeAllNodesSizeAct, SIGNAL(triggered()), this, SLOT(slotChangeAllNodesSize()) );

    changeAllNodesShapeAct = new QAction( tr("Change all Nodes Shape"),	this);
    changeAllNodesShapeAct->setStatusTip(tr("This option lets you change the shape of all nodes"));
    changeAllNodesShapeAct->setWhatsThis(tr("Nodes Shape\n\nThis option lets you change the shape of all nodes"));
    connect(changeAllNodesShapeAct, SIGNAL(triggered()), this, SLOT(slotChangeAllNodesShape()) );

    changeNodeBoxAct = new QAction(QIcon(":/images/box.png"), tr("Change Node Shape to Box"),this);
    changeNodeBoxAct->setStatusTip(tr("This option lets you change the shape of a node to a box"));
    changeNodeBoxAct->setWhatsThis(tr("Node as a box\n\nThis option lets you change the shape of a node to a box"));
    connect(changeNodeBoxAct, SIGNAL(triggered()), this, SLOT(slotChangeNodeBox()) );

    changeNodeTriangleAct = new QAction( tr("Change Node Shape to Triangle"),	this);
    changeNodeTriangleAct->setStatusTip(tr("This option lets you change the shape of a node to a box"));
    changeNodeTriangleAct->setWhatsThis(tr("Node as a box\n\nThis option lets you change the shape of a node to a box"));
    connect(changeNodeTriangleAct, SIGNAL(triggered()), this, SLOT(slotChangeNodeTriangle()) );

    changeNodeCircleAct = new QAction(QIcon(":/images/circle.png"), tr("Change Node Shape to Circle"),	this);
    changeNodeCircleAct->setStatusTip(tr("This option lets you change the shape of a node to a box"));
    changeNodeCircleAct->setWhatsThis(tr("Node as a box\n\nThis option lets you change the shape of a node to a box"));
    connect(changeNodeCircleAct, SIGNAL(triggered()), this, SLOT(slotChangeNodeCircle()) );

    changeNodeDiamondAct = new QAction(QIcon(":/images/diamond.png"), tr("Change Node Shape to Diamond"),	this);
    changeNodeDiamondAct->setStatusTip(tr("This option lets you change the shape of a node to a box"));
    changeNodeDiamondAct->setWhatsThis(tr("Node as a box\n\nThis option lets you change the shape of a node to a box"));
    connect(changeNodeDiamondAct, SIGNAL(triggered()), this, SLOT(slotChangeNodeDiamond()) );

    changeNodeEllipseAct = new QAction( tr("Change Node Shape to Ellipse"),	this);
    changeNodeEllipseAct->setStatusTip(tr("This option lets you change the shape of a node to a box"));
    changeNodeEllipseAct->setWhatsThis(tr("Node as a box\n\nThis option lets you change the shape of a node to a box"));
    connect(changeNodeEllipseAct, SIGNAL(triggered()), this, SLOT(slotChangeNodeEllipse()) );

    changeNumbersSizeAct = new QAction( tr("Change all Numbers Size"),	this);
    changeNumbersSizeAct->setStatusTip(tr("It lets you change the font size of the numbers of all nodes"));
    changeNumbersSizeAct->setWhatsThis(tr("Numbers Size\n\nChanges the size of the numbers of all nodes"));
    connect(changeNumbersSizeAct, SIGNAL(triggered()), this, SLOT(slotChangeNumbersSize()) );

    changeLabelsSizeAct = new QAction( tr("Change all Labels Size"), this);
    changeLabelsSizeAct->setStatusTip(tr("You can change the font size of the labels of all nodes"));
    changeLabelsSizeAct->setWhatsThis(tr("Labels Size\n\nChange the fontsize of the labels of all nodes"));
    connect(changeLabelsSizeAct, SIGNAL(triggered()), this, SLOT(slotChangeLabelsSize()) );

    addLinkAct = new QAction(QIcon(":/images/plines.png"), tr("Add Link"),this);
    addLinkAct->setShortcut(tr("Ctrl+L"));
    addLinkAct->setStatusTip(tr("Adds a Link to a Node"));
    addLinkAct->setWhatsThis(tr("Add Link\n\nAdds a Link to the network"));
    connect(addLinkAct, SIGNAL(triggered()), this, SLOT(slotAddLink()));

    removeLinkAct = new QAction(QIcon(":/images/disconnect.png"), tr("Remove"), this);
    removeLinkAct ->setShortcut(tr("Ctrl+Shift+L"));
    removeLinkAct->setStatusTip(tr("Removes a Link"));
    removeLinkAct->setWhatsThis(tr("Remove Link\n\nRemoves a Link from the network"));
    connect(removeLinkAct, SIGNAL(triggered()), this, SLOT(slotRemoveLink()));

    changeLinkLabelAct = new QAction(QIcon(":/images/letters.png"), tr("Change Label"), this);
    changeLinkLabelAct->setStatusTip(tr("Changes the Label of a Link"));
    changeLinkLabelAct->setWhatsThis(tr("Change Label\n\nChanges the label of a Link"));
    connect(changeLinkLabelAct, SIGNAL(triggered()), this, SLOT(slotChangeLinkLabel()));
    changeLinkLabelAct->setEnabled(false);

    changeLinkColorAct = new QAction(QIcon(":/images/colorize.png"),tr("Change Color"),	this);
    changeLinkColorAct->setStatusTip(tr("Changes the Color of a Link"));
    changeLinkColorAct->setWhatsThis(tr("Change Color\n\nChanges the Color of a Link"));
    connect(changeLinkColorAct, SIGNAL(triggered()), this, SLOT(slotChangeLinkColor()));

    changeLinkWeightAct = new QAction(tr("Change Weight"), this);
    changeLinkWeightAct->setStatusTip(tr("Changes the Weight of a Link"));
    changeLinkWeightAct->setWhatsThis(tr("Change Value\n\nChanges the Weight of a Link"));
    connect(changeLinkWeightAct, SIGNAL(triggered()), this, SLOT(slotChangeLinkWeight()));

    filterNodesAct = new QAction(tr("Filter Nodes"), this);
    filterNodesAct -> setEnabled(false);
    filterNodesAct->setStatusTip(tr("Filters Nodes of some value out of the network"));
    filterNodesAct->setWhatsThis(tr("Filter Nodes\n\nFilters Nodes of some value out of the network."));
    connect(filterNodesAct, SIGNAL(triggered()), this, SLOT(slotFilterNodes()));

    filterOrphanNodesAct = new QAction(tr("Filter Orphan Nodes"), this);
    filterOrphanNodesAct -> setEnabled(true);
    filterOrphanNodesAct -> setCheckable(true);
    filterOrphanNodesAct -> setChecked(false);
    filterOrphanNodesAct -> setStatusTip(tr("Filters Nodes with no edges"));
    filterOrphanNodesAct -> setWhatsThis(tr("Filter Orphan Nodes\n\n Enables or disables displaying of orphan nodes. Orphan nodes are those with no edges..."));
    connect(filterOrphanNodesAct, SIGNAL(triggered()), this, SLOT(slotFilterOrphanNodes()));

    filterEdgesAct = new QAction(tr("Filter Links"), this);
    filterEdgesAct -> setEnabled(true);
    filterEdgesAct -> setStatusTip(tr("Filters Links of some weight out of the network"));
    filterEdgesAct -> setWhatsThis(tr("Filter Links\n\nFilters Link of some specific weight out of the network."));
    connect(filterEdgesAct , SIGNAL(triggered()), this, SLOT(slotShowFilterEdgesDialog()));

    changeBackColorAct = new QAction(QIcon(":/images/color.png"), tr("Change Background Color"), this);
    changeBackColorAct->setStatusTip(tr("Click to change the background color"));
    changeBackColorAct->setWhatsThis(tr("Background\n\nChanges background color"));
    connect(changeBackColorAct, SIGNAL(triggered()), this, SLOT(slotBackgroundColor()));

    changeAllNodesColorAct = new QAction(QIcon(":/images/nodecolor.png"), tr("Change all Nodes Colors"),	this);
    changeAllNodesColorAct->setStatusTip(tr("Click to choose a new color for all nodes."));
    changeAllNodesColorAct->setWhatsThis(tr("All Nodes\n\nChanges all nodes color at once."));
    connect(changeAllNodesColorAct, SIGNAL(triggered()), this, SLOT(slotAllNodesColor()) );

    changeAllNumbersColorAct = new QAction( tr("Change all Numbers Colors"),	this);
    changeAllNumbersColorAct->setStatusTip(tr("Click to change the color of all numbers."));
    changeAllNumbersColorAct->setWhatsThis(tr("Numbers\n\nChanges the color of all numbers."));
    connect(changeAllNumbersColorAct, SIGNAL(triggered()), this, SLOT(slotAllNumbersColor()));

    changeAllLabelsColorAct = new QAction( tr("Change all Labels Colors"),	this);
    changeAllLabelsColorAct->setStatusTip(tr("Click to change the color of all node labels."));
    changeAllLabelsColorAct->setWhatsThis(tr("Numbers\n\nChanges the color of all node labels."));
    connect(changeAllLabelsColorAct, SIGNAL(triggered()), this, SLOT(slotAllLabelsColor()));

    changeAllLinksColorAct = new QAction( tr("Change all Links Colors"), this);
    changeAllLinksColorAct->setStatusTip(tr("Click to change the color of all links."));
    changeAllLinksColorAct->setWhatsThis(tr("Background\n\nChanges all links color"));
    connect(changeAllLinksColorAct, SIGNAL(triggered()), this, SLOT(slotAllLinksColor()));



    transformNodes2LinksAct = new QAction( tr("Transform Nodes to Links"),this);
    transformNodes2LinksAct->setStatusTip(tr("Transforms the network so that nodes become links and vice versa"));
    transformNodes2LinksAct->setWhatsThis(tr("Transform Nodes LinksAct\n\nTransforms network so that nodes become links and vice versa"));
    connect(transformNodes2LinksAct, SIGNAL(triggered()), this, SLOT(slotTransformNodes2Links()));

    symmetrizeAct= new QAction(QIcon(":/images/symmetrize.png"), tr("Symmetrize Links"), this);
    symmetrizeAct->setShortcut(tr("Shift+R"));
    symmetrizeAct->setStatusTip(tr("Makes all edges reciprocal (thus, a symmetric graph)."));
    symmetrizeAct->setWhatsThis(tr("Symmetrize Edges\n\nTransforms all arcs to double links (edges). The result is a symmetric network"));
    connect(symmetrizeAct, SIGNAL(triggered()), this, SLOT(slotSymmetrize()));




    /**
    Layout menu actions
    */
    strongColorationAct = new QAction ( tr("Strong Structural"), this);
    strongColorationAct -> setStatusTip( tr("Nodes are assigned the same color if they have identical in and out neighborhoods") );
    strongColorationAct -> setWhatsThis( tr("Click this to colorize nodes; Nodes are assigned the same color if they have identical in and out neighborhoods"));
    connect(strongColorationAct, SIGNAL(activated() ), this, SLOT(slotColorationStrongStructural()) );

    regularColorationAct = new QAction ( tr("Regular"), this);
    regularColorationAct -> setStatusTip( tr("Nodes are assigned the same color if they have neighborhoods of the same set of colors") );
    regularColorationAct -> setWhatsThis( tr("Click this to colorize nodes; Nodes are assigned the same color if they have neighborhoods of the same set of colors"));
    connect(regularColorationAct, SIGNAL(activated() ), this, SLOT(slotColorationRegular()) );//TODO

    randLayoutAct = new QAction( tr("Random"),this);
    randLayoutAct -> setShortcut(tr("Ctrl+0"));
    randLayoutAct -> setStatusTip(tr("Repositions all nodes in random places"));
    randLayoutAct -> setWhatsThis(tr("Random Layout\n\n Repositions all nodes in random places"));
    connect(randLayoutAct, SIGNAL(triggered()), this, SLOT(slotLayoutRandom()));

    randCircleLayoutAct = new QAction(tr("Random Circle"),	this);
    randCircleLayoutAct ->setStatusTip(tr("Repositions the nodes randomly on a circle"));
    randCircleLayoutAct->setWhatsThis(tr("Random Circle Layout\n\n Repositions the nodes randomly on a circle"));
    connect(randCircleLayoutAct, SIGNAL(triggered()), this, SLOT(slotLayoutRandomCircle()));


    layoutRadial_DC_Act = new QAction( tr("Degree Centrality"),	this);
    layoutRadial_DC_Act ->setShortcut(tr("Ctrl+Alt+1"));
    layoutRadial_DC_Act ->setStatusTip(tr("Repositions the nodes on circles of different radius according to their Degree Centrality."));
    layoutRadial_DC_Act->setWhatsThis(tr("Circle Degree Centrality Layout\n\n Repositions the nodes on circles of different radius. Nodes with higher Degree Centrality are situated towards the centre."));
    connect(layoutRadial_DC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutRadialByProminenceIndex()) );

    layoutRadial_CC_Act = new QAction( tr("Closeness Centrality"),	this);
    layoutRadial_CC_Act ->setShortcut(tr("Ctrl+Alt+2"));
    layoutRadial_CC_Act ->setStatusTip(tr("Repositions the nodes on circles of different radius according to their Closeness Centrality."));
    layoutRadial_CC_Act->setWhatsThis(tr("Circle Closeness Centrality Layout\n\n Repositions the nodes on circles of different radius. More Closeness Central Nodes are positioned towards the centre."));
    connect(layoutRadial_CC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutRadialByProminenceIndex()));

    layoutRadial_IRCC_Act = new QAction( tr("Influence Range Closeness Centrality"),	this);
    layoutRadial_IRCC_Act ->setShortcut(tr("Ctrl+Alt+3"));
    layoutRadial_IRCC_Act ->setStatusTip(tr("Repositions the nodes on circles of different radius according to their Ifluence Range Closeness Centrality."));
    layoutRadial_IRCC_Act->setWhatsThis(tr("Influence Range Closeness Centrality Circle Layout\n\n Repositions the nodes on circles of different radius. More IRCC prominent Nodes are positioned towards the centre."));
    connect(layoutRadial_IRCC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutRadialByProminenceIndex()));

    layoutRadial_BC_Act = new QAction( tr("Betweeness Centrality"), this);
    layoutRadial_BC_Act ->setShortcut(tr("Ctrl+Alt+4"));
    layoutRadial_BC_Act ->setStatusTip(tr("Repositions the nodes on circles of different radius according to their Betweeness Centrality."));
    layoutRadial_BC_Act->setWhatsThis(tr("Circle Betweeness Centrality Layout\n\n Repositions the nodes on circles of different radius. Nodes with higher Betweeness Centrality are situated towards the centre."));
    connect(layoutRadial_BC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutRadialByProminenceIndex()));

    layoutRadial_SC_Act = new QAction( tr("Stress Centrality"),	this);
    layoutRadial_SC_Act ->setShortcut(tr("Ctrl+Alt+5"));
    layoutRadial_SC_Act ->setStatusTip(tr("Repositions the nodes on circles of different radius according to their Stress Centrality."));
    layoutRadial_SC_Act->setWhatsThis(tr("Circle Stress Centrality Layout\n\n Repositions the nodes on circles of different radius. Nodes having higher Stress Centrality are situated towards the centre."));
    connect(layoutRadial_SC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutRadialByProminenceIndex()));

    layoutRadial_EC_Act = new QAction( tr("Eccentricity Centrality"),	this);
    layoutRadial_EC_Act ->setShortcut(tr("Ctrl+Alt+6"));
    layoutRadial_EC_Act  ->setStatusTip(tr("Repositions the nodes on circles of different radius according to their Eccentricity Centrality."));
    layoutRadial_EC_Act ->setWhatsThis(tr("Circle Eccentricity Centrality Layout\n\n Repositions the nodes on circles of different radius. Nodes with higher Eccentricity Centrality are situated towards the centre."));
    connect(layoutRadial_EC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutRadialByProminenceIndex()));

    layoutRadial_PC_Act = new QAction( tr("Power Centrality"),	this);
    layoutRadial_PC_Act ->setShortcut(tr("Ctrl+Alt+7"));
    layoutRadial_PC_Act  ->setStatusTip(tr("Repositions the nodes on circles of different radius according to their Power Centrality."));
    layoutRadial_PC_Act ->setWhatsThis(tr("Circle Power Centrality Layout\n\n Repositions the nodes on circles of different radius. Nodes with higher Power Centrality are situated towards the centre."));
    connect(layoutRadial_PC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutRadialByProminenceIndex()));


    layoutRadial_IC_Act = new QAction( tr("Information Centrality"),	this);
    layoutRadial_IC_Act ->setEnabled(true);
    layoutRadial_IC_Act ->setShortcut(tr("Ctrl+Alt+8"));
    layoutRadial_IC_Act ->setStatusTip(tr("Repositions the nodes on circles of different radius according to their Information Centrality."));
    layoutRadial_IC_Act->setWhatsThis(tr("Circle Information Centrality Layout\n\n Repositions the nodes on circles of different radius. Nodes with higher Information Centrality are situated towards the centre."));
    connect(layoutRadial_IC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutRadialByProminenceIndex()));


    layoutRadial_DP_Act = new QAction( tr("Degree Prestige"),	this);
    layoutRadial_DP_Act ->setShortcut(tr("Ctrl+Alt+D"));
    layoutRadial_DP_Act ->setStatusTip(tr("Repositions the nodes on circles of different radius according to their Degree Prestige."));
    layoutRadial_DP_Act->setWhatsThis(tr("Circle In-Degree Centrality Layout\n\n Repositions the nodes on circles of different radius. Nodes with higher Degree Prestige (inDegree) are situated towards the centre."));
    connect(layoutRadial_DP_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutRadialByProminenceIndex()));

    layoutRadial_PRP_Act = new QAction( tr("PageRank Prestige"),	this);
    layoutRadial_PRP_Act ->setEnabled(true);
    layoutRadial_PRP_Act ->setShortcut(tr("Ctrl+Alt+K"));
    layoutRadial_PRP_Act ->setStatusTip(tr("Repositions the nodes on circles of different radius according to their PageRank Prestige"));
    layoutRadial_PRP_Act->setWhatsThis(tr("Circle PageRank Centrality Layout\n\n Repositions the nodes on circles of different radius. More PageRank Central Nodes are positioned towards the centre."));
    connect(layoutRadial_PRP_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutRadialByProminenceIndex()));


    clearGuidesAct = new QAction(QIcon(":/images/gridlines.png"), tr("Remove Layout GuideLines"), this);
    clearGuidesAct ->setStatusTip(tr("Removes all layout guideLines from the canvas."));
    clearGuidesAct->setWhatsThis(tr("Remove GuideLines\n\n Removes any guidelines (circles or horizontal lines) created for the network layout."));


    levelOutDegreeLayoutAct  = new QAction( tr("Degree Centrality"),this);
    levelOutDegreeLayoutAct  ->setShortcut(tr("Ctrl+Shift+2"));
    levelOutDegreeLayoutAct  ->setStatusTip(tr("Repositions the nodes on levels of different height. More Out-Degree Central Nodes are situated on higher levels."));
    levelOutDegreeLayoutAct ->setWhatsThis(tr("Level Out-Degree Centrality Layout\n\n Repositions the nodes on levels of different height. More Out-Degree Central Nodes are situated on higher levels."));
    connect(levelOutDegreeLayoutAct , SIGNAL(triggered()), this, SLOT(slotLayoutLayeredCentralityOutDegree()));

    levelClosenessLayoutAct = new QAction( tr("Closeness Centrality"),	this);
    levelClosenessLayoutAct ->setShortcut(tr("Ctrl+Shift+3"));
    levelClosenessLayoutAct ->setStatusTip(tr("Repositions the nodes on levels of different height. More Closeness Central Nodes are situated on higher levels."));
    levelClosenessLayoutAct->setWhatsThis(tr("level Closeness Centrality Layout\n\n Repositions the nodes on levels of different height. More Closeness Central Nodes are situated on higher levels."));
    connect(levelClosenessLayoutAct, SIGNAL(triggered()), this, SLOT(slotLayoutLayeredCentralityCloseness()));

    levelBetweenessLayoutAct = new QAction( tr("Betweeness Centrality"),	this);
    levelBetweenessLayoutAct ->setShortcut(tr("Ctrl+Shift+4"));
    levelBetweenessLayoutAct ->setStatusTip(tr("Repositions the nodes on levels of different height. More Betweeness Central Nodes are situated on higher levels."));
    levelBetweenessLayoutAct->setWhatsThis(tr("level Betweeness Centrality Layout\n\n Repositions the nodes on levels of different height. More Betweeness Central Nodes are situated on higher levels."));
    connect(levelBetweenessLayoutAct, SIGNAL(triggered()), this, SLOT(slotLayoutLayeredCentralityBetweeness()));

    levelInformationLayoutAct = new QAction( tr("Information Centrality"),	this);
    levelInformationLayoutAct ->setShortcut(tr("Ctrl+Shift+9"));
    levelInformationLayoutAct -> setEnabled(false);
    levelInformationLayoutAct ->setStatusTip(tr("Repositions the nodes on levels of different height. More Informational Central Nodes are situated on higher levels."));
    levelInformationLayoutAct->setWhatsThis(tr("Level Informational Centrality Layout\n\n Repositions the nodes on levels of different height. More Informational Central Nodes are situated on higher levels."));
    connect(levelInformationLayoutAct, SIGNAL(triggered()), this, SLOT(slotLayoutLayeredCentralityInformation()));


    levelInDegreeLayoutAct = new QAction( tr("Degree Prestige"),this);
    levelInDegreeLayoutAct ->setShortcut(tr("Ctrl+Shift+1"));
    levelInDegreeLayoutAct ->setStatusTip(tr("Repositions the nodes on layers or levels of different height. More In-Degree Prestigious Nodes are situated on higher layers."));
    levelInDegreeLayoutAct->setWhatsThis(tr("Level Degree Prestige Layout\n\n Repositions the nodes on layers of different height. More In-Degree Prestigious Nodes are situated on higher layers."));
    connect(levelInDegreeLayoutAct, SIGNAL(triggered()), this, SLOT(slotLayoutLayeredCentralityInDegree()));


    springLayoutAct= new QAction(tr("Spring Embedder"), this);
    springLayoutAct->setShortcut(tr("Alt+1"));
    springLayoutAct->setCheckable(true);
    springLayoutAct->setChecked(false);
    springLayoutAct->setStatusTip(tr("All nodes repel each other while the connected ones are attracted as if connected by springs."));
    springLayoutAct->setWhatsThis(tr("Spring Embedder Layout\n\n In this model, nodes are regarded as physical bodies (i.e. electrons) which exert repelling forces to each other, while edges are springs connecting adjacents nodes. Non-adjacent nodes repel each other while connected nodes are The algorithm continues until the system retains an equilibrium state in which all forces cancel each other. "));
    connect(springLayoutAct, SIGNAL(triggered(bool)), this, SLOT(slotLayoutSpringEmbedder(bool)));

    FRLayoutAct= new QAction( tr("Fruchterman-Reingold"),	this);
    FRLayoutAct->setShortcut(tr("Alt+2"));
    FRLayoutAct->setCheckable(true);
    FRLayoutAct->setChecked(false);
    FRLayoutAct->setStatusTip(tr("Repelling forces between all nodes, and attracting forces between adjacent nodes."));
    FRLayoutAct->setWhatsThis(tr("Fruchterman-Reingold Layout\n\n Embeds a layout all nodes according to a model in which	repelling forces are used between every pair of nodes, while attracting forces are used only between adjacent nodes. The algorithm continues until the system retains its equilibrium state where all forces cancel each other."));
    connect(FRLayoutAct, SIGNAL(triggered()), this, SLOT(slotLayoutFruchterman()));


    zoomInAct = new QAction(QIcon(":/images/zoomin.png"), tr("Zoom &in"),  this);
    zoomInAct->setShortcut(tr("Ctrl++"));
    zoomInAct->setToolTip(tr("Zoom in (Ctrl++)"));
    zoomInAct->setStatusTip(tr("Zooms inside the actual network."));
    zoomInAct->setWhatsThis(tr("Zoom In.\n\nZooms in. What else did you expect?"));


    zoomOutAct = new QAction(QIcon(":/images/zoomout.png"), tr("Zoom &out"),  this);
    zoomOutAct->setShortcut(tr("Ctrl+-"));
    zoomOutAct->setToolTip(tr("Zoom out (Ctrl+-)"));
    zoomOutAct->setStatusTip(tr("Zooms out of the actual network."));
    zoomOutAct->setWhatsThis(tr("Zoom out.\n\nZooms out. What else did you expect?"));

    nodeSizeProportionalOutDegreeAct= new QAction(QIcon(":/images/nodeout.png"),tr("Node size according to outDegree"), this);
    nodeSizeProportionalOutDegreeAct->setShortcut(tr("Alt+3"));
    nodeSizeProportionalOutDegreeAct->setStatusTip(tr("Resizes all nodes according to their out edges."));
    nodeSizeProportionalOutDegreeAct->setWhatsThis(tr("NodeSize = F (OutDegree) \n\n Adjusts the size of each node according to their out-edges (OutDegree). The more out-likned a node is, the bigger will appear..."));
    nodeSizeProportionalOutDegreeAct->setCheckable(true);
    nodeSizeProportionalOutDegreeAct->setChecked(false);
    connect(nodeSizeProportionalOutDegreeAct, SIGNAL(triggered(bool)), this, SLOT(slotLayoutNodeSizeProportionalOutEdges(bool)));


    nodeSizeProportionalInDegreeAct= new QAction(QIcon(":/images/nodein.png"),tr("Node size according to InDegree"), this);
    nodeSizeProportionalInDegreeAct->setShortcut(tr("Alt+4"));
    nodeSizeProportionalInDegreeAct->setStatusTip(tr("Resizes all nodes according to their in edges."));
    nodeSizeProportionalInDegreeAct->setWhatsThis(tr("NodeSize = F (InDegree) \n\n This method adjusts the size of each node according to their in-edges (InDegree). The more in-linked a node is, the bigger will appear..."));
    nodeSizeProportionalInDegreeAct->setCheckable(true);
    nodeSizeProportionalInDegreeAct->setChecked(false);
    connect(nodeSizeProportionalInDegreeAct, SIGNAL(triggered(bool)), this, SLOT(slotLayoutNodeSizeProportionalInEdges(bool)));



    /**
    Statistics menu actions
    */

    symmetryAct = new QAction(QIcon(":/images/symmetry.png"), tr("Symmetry"), this);
    symmetryAct ->setShortcut(tr("Shift+S"));
    symmetryAct->setStatusTip(tr("Checks whether the network is symmetric or not"));
    symmetryAct->setWhatsThis(tr("Symmetry\n\n A network is symmetric when all edges are reciprocal, or, in mathematical language, when the adjacency matrix is symmetric."));
    connect(symmetryAct, SIGNAL(triggered()), this, SLOT(slotCheckSymmetry()));

    invertAdjMatrixAct = new QAction(QIcon(":/images/symmetry.png"), tr("Invert Adjacency Matrix"), this);
    invertAdjMatrixAct ->setShortcut(tr("Shift+I"));
    invertAdjMatrixAct->setStatusTip(tr("Inverts the adjacency matrix"));
    invertAdjMatrixAct->setWhatsThis(tr("Invert  Adjacency Matrix \n\n Inverts the adjacency matrix using linear algebra methods."));
    connect(invertAdjMatrixAct, SIGNAL(triggered()), this, SLOT(slotInvertAdjMatrix()));

    graphDistanceAct = new QAction(QIcon(":/images/distance.png"),  tr("Geodesic Distance"), this);
    graphDistanceAct ->setShortcut(tr("Ctrl+G"));
    graphDistanceAct->setStatusTip(tr("Calculates the length of the shortest path between two nodes..."));
    graphDistanceAct->setWhatsThis(tr("Geodesic Distance\n\n In graph theory, the distance (or geodesic distance) of two nodes is the length (number of edges) of the shortest path between them."));
    connect(graphDistanceAct, SIGNAL(triggered()), this, SLOT(slotGraphDistance()));

    distanceMatrixAct = new QAction(QIcon(":/images/dm.png"), tr("Geodesic Distance &Matrix"),this);
    distanceMatrixAct ->setShortcut(tr("Shift+G"));
    distanceMatrixAct->setStatusTip(tr("The matrix of graph geodesic distances between all nodes"));
    distanceMatrixAct->setWhatsThis(tr("Distance Matrix\n\n A distance matrix is a NxN matrix, where the (i,j) element is the geodesic distance from node i to node j. The geodesic distance of two nodes is the length of the shortest path between them."));
    connect(distanceMatrixAct, SIGNAL(triggered()), this, SLOT( slotViewDistanceMatrix() ) );

    geodesicsMatrixAct = new QAction(QIcon(":/images/dm.png"), tr("Number of Geodesic &Paths Matrix"),this);
    geodesicsMatrixAct ->setShortcut(tr("Ctrl+Shift+G"));
    geodesicsMatrixAct->setStatusTip(tr("The number of geodesic paths between each pair of nodes "));
    geodesicsMatrixAct->setWhatsThis(tr("Number of Geodesics\n\n Displays a NxN matrix, where the (i,j) element is the number of geodesic paths between node i and node j. A geodesic path of two nodes is the shortest path between them."));
    connect(geodesicsMatrixAct, SIGNAL(triggered()), this, SLOT( slotViewNumberOfGeodesicsMatrix()) );

    diameterAct = new QAction(QIcon(":/images/diameter.png"), tr("Diameter"),this);
    diameterAct ->setShortcut(tr("Ctrl+D"));
    diameterAct->setStatusTip(tr("The diameter of the network."));
    diameterAct->setWhatsThis(tr("Diameter\n\n The Diameter of a network is the maximum graph distance (maximum shortest path length) between any two nodes of the network."));
    connect(diameterAct, SIGNAL(triggered()), this, SLOT(slotDiameter()));

    averGraphDistanceAct = new QAction(QIcon(":/images/avdistance.png"), tr("Average Geodesic Distance"),this);
    averGraphDistanceAct ->setShortcut(tr("Ctrl+B"));
    averGraphDistanceAct->setStatusTip(tr("The average shortest path length."));
    averGraphDistanceAct->setWhatsThis(tr("Average Geodesic Distance\n\n This the average length of all shortest paths between the connected pair of nodes of the network."));
    connect(averGraphDistanceAct, SIGNAL(triggered()), this, SLOT(slotAverageGraphDistance()));

    eccentricityAct = new QAction(QIcon(":/images/eccentricity.png"), tr("Eccentricity"),this);
    eccentricityAct->setShortcut(tr("Ctrl+E"));
    eccentricityAct->setStatusTip(tr("Eccentricity indices for each node and group Eccentricity"));
    eccentricityAct->setWhatsThis(tr("Eccentricity\n\n The eccentricity or association number of each node i is the largest geodesic distance (i,j) between node i and every other node j. Therefore, it reflects how far, at most, is each node from every other node. \n\nThis index can be calculated in both graphs and digraphs but is usually best suited for undirected graphs. It can also be calculated in weighted graphs although the weight of each edge (v,u) in E is always considered to be 1."));
    connect(eccentricityAct, SIGNAL(triggered()), this, SLOT(slotEccentricity()));


    connectednessAct = new QAction(QIcon(":/images/distance.png"),  tr("Connectedness"), this);
    connectednessAct ->setShortcut(tr("Ctrl+Shift+C"));
    connectednessAct->setStatusTip(tr("Checks whether the network is a connected "
                                      "graph, a weakly connected digraph or "
                                      "a disconnected graph/digraph..."));
    connectednessAct->setWhatsThis(tr("Connectedness\n\n In graph theory, a "
                                      "graph is <b>connected</b> if there is a "
                                      "path between every pair of nodes. \n"
                                      "A digraph is <b>strongly connected</b> "
                                      "if there the a path from i to j and "
                                      "from j to i for all nodes (i,j).\n"
                                      "(i,j).\n"
                                      "A digraph is weakly connected if at least "
                                      "a pair of nodes are joined by a semipath.\n"
                                      "A digraph or a graph is disconnected if "
                                      "at least one node is isolate."
                                      ));
    connect(connectednessAct, SIGNAL(triggered()), this, SLOT(slotConnectedness()));


    walksAct = new QAction(QIcon(":/images/walk.png"), tr("Number of Walks Matrix"),this);
    walksAct->setShortcut(tr("Ctrl+W"));
    walksAct->setStatusTip(tr("The number of walks of a given length between any nodes."));
    walksAct->setWhatsThis(tr("Walks\n\n A walk is a sequence of alternating vertices and edges such as v<sub>0</sub>e<sub>1</sub>, v<sub>1</sub>e<sub>2</sub>, v<sub>2</sub>e<sub>3</sub>, …, e<sub>k</sub>v<sub>k</sub>, where each edge, e<sub>i</sub> is defined as e<sub>i</sub> = {v<sub>i-1</sub>, v<sub>i</sub>}. This function counts the number of walks of a given length between each pair of nodes, by studying the powers of the sociomatrix.\n "));
    connect(walksAct, SIGNAL(triggered()), this, SLOT(slotNumberOfWalks() )  );

    totalWalksAct = new QAction(QIcon(":/images/walk.png"), tr("Total Number of Walks Matrix"),this);
    totalWalksAct->setShortcut(tr("Ctrl+Shift+W"));
    totalWalksAct->setStatusTip(tr("Calculates the total number of walks of every possible length between all nodes"));
    totalWalksAct->setWhatsThis(tr("Walks\n\n A walk is a sequence of alternating vertices and edges such as v<sub>0</sub>e<sub>1</sub>, v<sub>1</sub>e<sub>2</sub>, v<sub>2</sub>e<sub>3</sub>, …, e<sub>k</sub>v<sub>k</sub>, where each edge, e<sub>i</sub> is defined as e<sub>i</sub> = {v<sub>i-1</sub>, v<sub>i</sub>}. This function counts the number of walks of any length between each pair of nodes, by studying the powers of the sociomatrix\n "));
    connect(totalWalksAct, SIGNAL(triggered()), this, SLOT(slotTotalNumberOfWalks() )  );


    reachabilityMatrixAct = new QAction(QIcon(":/images/walk.png"), tr("Reachability Matrix"),this);
    reachabilityMatrixAct->setShortcut(tr("Ctrl+Shift+R"));
    reachabilityMatrixAct->setStatusTip(tr("Calculates the Reachability Matrix for the loaded network."));
    reachabilityMatrixAct->setWhatsThis(tr("Reachability Matrix\n\n     Calculates the reachability matrix X<sup>R</sup> of the graph where the {i,j} element is 1 if the vertices i and j are reachable. \n\n Actually, this just checks whether the corresponding element of Distances matrix is not zero.\n "));
    connect(reachabilityMatrixAct, SIGNAL(triggered()), this, SLOT(slotReachabilityMatrix() )  );

    cliquesAct = new QAction(QIcon(":/images/triangle.png"), tr("Number of Cliques"),this);
    cliquesAct->setShortcut(tr("Ctrl+T"));
    cliquesAct->setStatusTip(tr("The number of cliques (triangles) of each node v."));
    cliquesAct->setWhatsThis(tr("Number of Cliques\n\n A triangle is a complete subgraph of three nodes of G. This method calculates the number of triangles of each node v is defined as delta(v) = |{{u, w} in E : {v, u} in E and {v, w} in E}|.  \n "));
    connect(cliquesAct, SIGNAL(triggered()), this, SLOT(slotNumberOfCliques() )  );


    clusteringCoefAct = new QAction(QIcon(":/images/clique.png"), tr("Clustering Coefficient"),this);
    clusteringCoefAct ->setShortcut(tr("Ctrl+C"));
    clusteringCoefAct->setStatusTip(tr("The average Clustering Coefficient of the network."));
    clusteringCoefAct->setWhatsThis(tr("Clustering Coefficient\n\n The Clustering Coefficient of a vertex quantifies how close the vertex and its neighbors are to being a clique. \n "));
    connect(clusteringCoefAct, SIGNAL(triggered()), this, SLOT(slotClusteringCoefficient() )  );


    triadCensusAct = new QAction(QIcon(":/images/clique.png"), tr("Triad Census"),this);
    triadCensusAct->setShortcut(tr("Ctrl+Shift+T"));
    triadCensusAct->setStatusTip(tr("Conducts a triad census for the active network."));
    triadCensusAct->setWhatsThis(tr("Triad Census\n\n A triad census counts all the different kinds of observed triads within a network and codes them according to their number of mutual, asymmetric and non-existent dyads. \n "));
    connect(triadCensusAct, SIGNAL(triggered()), this, SLOT(slotTriadCensus() )  );

    cDegreeAct = new QAction(tr("Degree Centrality (DC)"),this);
    cDegreeAct->setShortcut(tr("Ctrl+1"));
    cDegreeAct->setStatusTip(tr("Degree Centrality indices and group Degree Centralization."));
    cDegreeAct->setWhatsThis(tr("Degree Centrality (DC)\n\n For each node v, the DC index is the number of edges attached to it (in undirected graphs) or the total numnber of arcs (outLinks) starting from it (in digraphs). This is oftenly considered a measure of actor activity. \n\nThis index can be calculated in both graphs and digraphs but is usually best suited for undirected graphs. It can also be calculated in weighted graphs. In weighted relations, ODC is the sum of weights of all edges/outLinks attached to v."));
    connect(cDegreeAct, SIGNAL(triggered()), this, SLOT(slotCentralityDegree()));


    cClosenessAct = new QAction(tr("Closeness Centrality (CC)"), this);
    cClosenessAct->setShortcut(tr("Ctrl+2"));
    cClosenessAct->setStatusTip(tr("Closeness Centrality indices and group Closeness Centralization."));
    cClosenessAct->setWhatsThis(tr("Closeness Centrality (CC)\n\n "
                                   "For each node v, CC the inverse sum of "
                                   "the shortest distances between v and every other node. CC is "
                                   "interpreted as the ability to access information through the "
                                   "\"grapevine\" of network members. Nodes with high closeness "
                                   "centrality are those who can reach many other nodes in few steps. "
                                   "\n\nThis index can be calculated in both graphs and digraphs. "
                                   "It can also be calculated in weighted graphs although the weight of "
                                   "each edge (v,u) in E is always considered to be 1. "));
    connect(cClosenessAct, SIGNAL(triggered()), this, SLOT(slotCentralityCloseness()));

    cInfluenceRangeClosenessAct = new QAction(tr("Influence Range Closeness Centrality (IRCC)"), this);
    cInfluenceRangeClosenessAct->setShortcut(tr("Ctrl+3"));
    cInfluenceRangeClosenessAct->setStatusTip(tr("Closeness Centrality indices focusing on how proximate each node is"
                                                 "to the nodes in its influence range"));
    cInfluenceRangeClosenessAct->setWhatsThis(tr("Influence Range Closeness Centrality (IRCC)\n\n "
                                                 "For each node v, IRCC is the standardized inverse average distance "
                                                 "between v and every reachable node.\n"
                                                 "This improved CC index is optimized for graphs and directed graphs which "
                                                 "are not strongly connected. Unlike the ordinary CC, which is the inverted "
                                                 "sum of distances from node v to all others (thus undefined if a node is isolated "
                                                 "or the digraph is not strongly connected), IRCC considers only "
                                                 "distances from node v to nodes in its influence range J (nodes reachable from v). "
                                                 "The IRCC formula used is the ratio of the fraction of nodes reachable by v "
                                                 "(|J|/(n-1)) to the average distance of these nodes from v (sum(d(v,j))/|J|"));
    connect(cInfluenceRangeClosenessAct, SIGNAL(triggered()), this, SLOT(slotCentralityClosenessInfluenceRange()));

    cBetweenessAct = new QAction(tr("Betweeness Centrality (BC)"), this);
    cBetweenessAct->setShortcut(tr("Ctrl+4"));
    cBetweenessAct->setWhatsThis(tr("Betweeness Centrality (BC)\n\n For each node v, BC is the ratio of all geodesics between pairs of nodes which run through v. It reflects how often an node lies on the geodesics between the other nodes of the network. It can be interpreted as a measure of control. A node which lies between many others is assumed to have a higher likelihood of being able to control information flow in the network. \n\n Note that betweeness centrality assumes that all geodesics have equal weight or are equally likely to be chosen for the flow of information between any two nodes. This is reasonable only on \"regular\" networks where all nodes have similar degrees. On networks with significant degree variance you might want to try informational centrality instead. \n\nThis index can be calculated in both graphs and digraphs but is usually best suited for undirected graphs. It can also be calculated in weighted graphs although the weight of each edge (v,u) in E is always considered to be 1."));
    cBetweenessAct->setStatusTip(tr("Betweeness Centrality indices and group Betweeness Centralization."));
    connect(cBetweenessAct, SIGNAL(triggered()), this, SLOT(slotCentralityBetweeness()));

    cStressAct = new QAction(tr("Stress Centrality (SC)"), this);
    cStressAct->setShortcut(tr("Ctrl+5"));
    cStressAct->setStatusTip(tr("Stress Centrality indices and group Stress Centralization."));
    cStressAct->setWhatsThis(tr("Stress Centrality (SC)\n\n For each node v, SC is the total number of geodesics between all other nodes which run through v. A node with high SC is considered 'stressed', since it is traversed by a high number of geodesics. When one node falls on all other geodesics between all the remaining (N-1) nodes, then we have a star graph with maximum Stress Centrality. \n\nThis index can be calculated in both graphs and digraphs but is usually best suited for undirected graphs. It can also be calculated in weighted graphs although the weight of each edge (v,u) in E is always considered to be 1."));
    connect(cStressAct, SIGNAL(triggered()), this, SLOT(slotCentralityStress()));


    cEccentAct = new QAction(tr("Eccentricity Centrality (EC)"), this);
    cEccentAct->setShortcut(tr("Ctrl+6"));
    cEccentAct->setStatusTip(tr("Eccentricity Centrality indices for each node."));
    cEccentAct->setWhatsThis(
                tr("Eccentricity Centrality (EC)\n\n For each node i, "
                   "the EC is the inverse of the maximum geodesic distance "
                   "of that v to all other nodes in the network. \n"
                   "Nodes with high EC have short distances to all other nodes "
                   "This index can be calculated in both graphs and digraphs "
                   "but is usually best suited for undirected graphs. "
                   "It can also be calculated in weighted graphs although the weight of each edge (v,u) in E is always considered to be 1."));
    connect(cEccentAct, SIGNAL(triggered()), this, SLOT(slotCentralityEccentricity()));


    cPowerAct = new QAction(tr("Power Centrality (PC)"), this);
    cPowerAct->setShortcut(tr("Ctrl+7"));
    cPowerAct->setStatusTip(tr("Calculate and display Power Centrality indices (aka Gil-Schmidt Power Centrality) and group Power Centralization"));
    cPowerAct->setWhatsThis(tr("Power Centrality (PC)\n\n For each node v, this index sums its degree (with weight 1), with the size of the 2nd-order neighbourhood (with weight 2), and in general, with the size of the kth order neighbourhood (with weight k). Thus, for each node in the network the most important other nodes are its immediate neighbours and then in decreasing importance the nodes of the 2nd-order neighbourhood, 3rd-order neighbourhood etc. For each node, the sum obtained is normalised by the total numbers of nodes in the same component minus 1. Power centrality has been devised by Gil-Schmidt. \n\nThis index can be calculated in both graphs and digraphs but is usually best suited for undirected graphs. It can also be calculated in weighted graphs although the weight of each edge (v,u) in E is always considered to be 1 (therefore not considered)."));
    connect(cPowerAct, SIGNAL(triggered()), this, SLOT(slotCentralityPower()));


    cInformationAct = new QAction(tr("Information Centrality (IC)"),	this);
    cInformationAct->setShortcut(tr("Ctrl+8"));
    cInformationAct->setEnabled(true);
    cInformationAct->setStatusTip(tr("Calculate and display Information Centrality indices and group Information Centralization"));
    cInformationAct->setWhatsThis(tr("Information Centrality (IC)\n\n Information centrality counts all paths between nodes weighted by strength of tie and distance. This centrality  measure developed by Stephenson and Zelen (1989) focuses on how information might flow through many different paths. \n\nThis index should be calculated only for  graphs. \n\n Note: To compute this index, SocNetV drops all isolated nodes."));
    connect(cInformationAct, SIGNAL(triggered()), this, SLOT(slotCentralityInformation()));

    cInDegreeAct = new QAction(tr("Degree Prestige (DP)"),	 this);
    cInDegreeAct->setStatusTip(tr("Degree Prestige (InDegree) indices "));
    cInDegreeAct->setShortcut(tr("Ctrl+Shift+D"));
    cInDegreeAct->setWhatsThis(tr("InDegree (Degree Prestige)\n\n For each node k, this the number of arcs ending at k. Nodes with higher in-degree are considered more prominent among others. In directed graphs, this index measures the prestige of each node/actor. Thus it is called Degree Prestige. Nodes who are prestigious tend to receive many nominations or choices (in-links). The largest the index is, the more prestigious is the node. \n\nThis index can be calculated only for digraphs. In weighted relations, DP is the sum of weights of all arcs/inLinks ending at node v."));
    connect(cInDegreeAct, SIGNAL(triggered()), this, SLOT(slotPrestigeDegree()));

    cPageRankAct = new QAction(tr("PageRank Prestige (PRP)"),	this);
    cPageRankAct->setShortcut(tr("Ctrl+Shift+K"));
    cPageRankAct->setEnabled(true);
    cPageRankAct->setStatusTip(tr("Calculate and display PageRank Prestige"));
    cPageRankAct->setWhatsThis(tr("PageRank Prestige\n\n An importance ranking for each node based on the link structure of the network. PageRank, developed by Page and Brin (1997), focuses on how nodes are connected to each other, treating each link from a node as a citation/backlink/vote to another. In essence, for each node PageRank counts all backlinks to it, but it does so by not counting all links equally while it normalizes each link from a node by the total number of links from it. PageRank is calculated iteratively and it corresponds to the principal eigenvector of the normalized link matrix. \n\nThis index can be calculated in both graphs and digraphs but is usually best suited for directed graphs since it is a prestige measure. It can also be calculated in weighted graphs. In weighted relations, each backlink to a node v from another node u is considered to have weight=1 but it is normalized by the sum of outLinks weights (outDegree) of u. Therefore, nodes with high outLink weights give smaller percentage of their PR to node v."));
    connect(cPageRankAct, SIGNAL(triggered()), this, SLOT(slotPrestigePageRank()));

    cProximityPrestigeAct = new QAction(tr("Proximity Prestige (PP)"),	this);
    cProximityPrestigeAct->setShortcut(tr("Ctrl+Shift+P"));
    cProximityPrestigeAct->setEnabled(true);
    cProximityPrestigeAct->setStatusTip(tr("Calculate and display Proximity Prestige (digraphs only)"));
    cProximityPrestigeAct->setWhatsThis(tr("Proximity Prestige (PP) \n\n This index measures how proximate a node v is to the nodes in its influence domain I (the influence domain I of a node is the number of other nodes that can reach it). In PP calculation, proximity is based on distances to rather than distances from node v. To put it simply, in PP what matters is how close are all the other nodes to node v. \n\nThe algorithm takes the average distance to node v of all nodes in its influence domain, standardizes it by multiplying with (N-1)/I and takes its reciprocal. In essence, the formula SocNetV uses to calculate PP for every node v is the ratio of the fraction of nodes that can reach node v, to the average distance of that noeds to v: PP = (I/(N-1))/(sum{d(u,v)}/I) where the sum is over all nodes in I."));
    connect(cProximityPrestigeAct, SIGNAL(triggered()), this, SLOT(slotPrestigeProximity()));

    /**
    Options menu actions
    */
    displayNodeNumbersAct = new QAction( tr("Display Num&bers"), this );
    displayNodeNumbersAct->setStatusTip(tr("Toggles displaying of node numbers"));
    displayNodeNumbersAct->setWhatsThis(tr("Display Numbers\n\nEnables/disables node numbers"));
    displayNodeNumbersAct->setCheckable (true);
    displayNodeNumbersAct->setChecked(true);
    connect(displayNodeNumbersAct, SIGNAL(toggled(bool)), this, SLOT(slotDisplayNodeNumbers(bool)));

    displayNodeLabelsAct= new QAction(tr("Display Labels"),	this );
    displayNodeLabelsAct->setStatusTip(tr("Toggles displaying of node labels"));
    displayNodeLabelsAct->setWhatsThis(tr("Display Labels\n\nEnables/disables node labels"));
    displayNodeLabelsAct->setCheckable (true);
    displayNodeLabelsAct->setChecked(false);
    connect(displayNodeLabelsAct, SIGNAL(toggled(bool)), this, SLOT(slotDisplayNodeLabels(bool)));


    displayNumbersInsideNodesAct= new QAction(tr("Display Numbers Inside Nodes"),	this );
    displayNumbersInsideNodesAct->setStatusTip(tr("Toggles displaying numbers inside nodes"));
    displayNumbersInsideNodesAct->setWhatsThis(tr("Display Numbers Inside Nodes\n\nTurns on/off displaying nodenumbers inside nodes"));
    displayNumbersInsideNodesAct->setCheckable (true);
    displayNumbersInsideNodesAct->setChecked(false);
    connect(displayNumbersInsideNodesAct, SIGNAL(toggled(bool)), this, SLOT(slotDisplayNumbersInsideNodes(bool)));


    displayLinksAct = new QAction(tr("Display Links"),	this);
    displayLinksAct->setStatusTip(tr("Toggle to display or not links"));
    displayLinksAct->setWhatsThis(tr("Display Links\n\nClick to enable or disable displaying of links"));
    displayLinksAct->setCheckable(true);
    displayLinksAct->setChecked(true);
    connect(displayLinksAct, SIGNAL(toggled(bool)), this, SLOT(slotDisplayLinks(bool)) );

    displayLinksWeightNumbersAct = new QAction(tr("Display Link Weights"),	this);
    displayLinksWeightNumbersAct->setStatusTip(tr("Toggles displaying of numbers of links weights"));
    displayLinksWeightNumbersAct->setWhatsThis(tr("Display Weight Numbers\n\nClick to enable or disable displaying numbers of links weight"));
    displayLinksWeightNumbersAct->setCheckable(true);
    displayLinksWeightNumbersAct->setChecked(false);
    connect(displayLinksWeightNumbersAct, SIGNAL(toggled(bool)), this, SLOT(slotDisplayLinksWeightNumbers(bool)) );

    displayLinksArrowsAct = new QAction( tr("Display Arrows"),this);
    displayLinksArrowsAct->setStatusTip(tr("Toggles displaying of arrows in the end of links"));
    displayLinksArrowsAct->setWhatsThis(tr("Display Arrows\n\nClick to enable or disable displaying of arrows in the end of links"));
    displayLinksArrowsAct->setCheckable(true);
    displayLinksArrowsAct->setChecked(true);
    connect(displayLinksArrowsAct, SIGNAL(toggled(bool)), this, SLOT(slotDisplayLinksArrows(bool)) );

    drawLinksWeightsAct = new QAction( tr("Thickness=Weight"), this);
    drawLinksWeightsAct->setStatusTip(tr("Draws links as thick as their weights (if specified)"));
    drawLinksWeightsAct->setWhatsThis(tr("Draw As Thick As Weights\n\nClick to toggle having all links as thick as their weight (if specified)"));
    drawLinksWeightsAct->setCheckable(true);
    drawLinksWeightsAct->setChecked(false);
    drawLinksWeightsAct->setEnabled(false);
    connect(drawLinksWeightsAct, SIGNAL(toggled(bool)), this, SLOT(slotDrawLinksThickAsWeights()) );

    drawLinksBezier = new QAction( tr("Bezier Curves"),	this);
    drawLinksBezier->setStatusTip(tr("Draws links as Bezier curves"));
    drawLinksBezier->setWhatsThis(tr("Links Bezier\n\nEnables/Disables drawing Links as Bezier curves."));
    drawLinksBezier->setCheckable(true);
    drawLinksBezier->setChecked (false);
    drawLinksBezier->setEnabled(false);
    connect(drawLinksBezier, SIGNAL(toggled(bool)), this, SLOT(slotDrawLinksBezier(bool)) );


    /**
    Options > View menu actions
    */

    antialiasingAct = new QAction(tr("Anti-Aliasing"), this);
    antialiasingAct ->setShortcut(tr("F8"));
    antialiasingAct ->setStatusTip(tr("Enables/disables anti-aliasing"));
    antialiasingAct ->setWhatsThis(tr("Enable or disable Anti-Aliasing\n\n Anti-aliasing is a technique which makes nodes, lines and text, smoother and fancier. But it comes at the cost of speed..."));
    antialiasingAct ->setCheckable(true);
    antialiasingAct ->setChecked (true);
    connect(antialiasingAct , SIGNAL(toggled(bool)), this, SLOT(slotAntialiasing(bool)));


    showProgressBarAct = new QAction(tr("Progress Bars"), this);
    showProgressBarAct ->setShortcut(tr("F10"));
    showProgressBarAct->setStatusTip(tr("Enables/disables Progress Bars"));
    showProgressBarAct->setWhatsThis(tr("Enable or disable Progress Bars\n\nProgress Bars may appear during time-cost operations. Enabling progressBar has a significant cpu cost but lets you know about the progress of a given operation."));
    showProgressBarAct->setCheckable(true);
    showProgressBarAct->setChecked (true);
    connect(showProgressBarAct, SIGNAL(toggled(bool)), this, SLOT(slotShowProgressBar(bool)));

    printDebugAct = new QAction(tr("Debug Messages"),	this);
    printDebugAct ->setShortcut(tr("F9"));
    printDebugAct->setStatusTip(tr("Enables/disables printing debug messages to stdout"));
    printDebugAct->setWhatsThis(tr("Enables or disable Debug Messages\n\nPrinting debug messages to strerr. Enabling has a significant cpu cost but lets you know what SocNetV is actually doing."));
    printDebugAct->setCheckable(true);
    printDebugAct->setChecked (false);
    printDebug=false;
    connect(printDebugAct, SIGNAL(toggled(bool)), this, SLOT(slotPrintDebug(bool)));



    viewToolBar = new QAction(tr("Toolbar"), this);
    viewToolBar->setStatusTip(tr("Enables/disables the toolbar"));
    viewToolBar->setWhatsThis(tr("Enable or disable Toolbar\n\nThe toolbar is the widget right below the menu, and carries useful icons. You can disable it if you like..."));
    viewToolBar->setCheckable(true);
    viewToolBar->setChecked(true);
    connect(viewToolBar, SIGNAL(toggled(bool)), this, SLOT(slotViewToolBar(bool)));

    viewStatusBar = new QAction(tr("Statusbar"),	this);
    viewStatusBar->setStatusTip(tr("Enables/disables the statusbar"));
    viewStatusBar->setWhatsThis(tr("Enable or disable Statusbar\n\nThe statusbar is the widget at the bottom of the window, where messages appear. You might want to disable it..."));
    viewStatusBar->setCheckable(true);
    viewStatusBar->setChecked(true);
    connect(viewStatusBar, SIGNAL(toggled(bool)), this, SLOT(slotViewStatusBar(bool)));

    backgroundImageAct = new QAction(tr("Background Image"),	this);
    backgroundImageAct->setStatusTip(tr("Enables/disables displaying a user-defined custom image in the background"));
    backgroundImageAct->setWhatsThis(tr("Enable or disable background image\n\n If you enable it, you will be asked for a image file, which will be displayed in the background instead of plain color.."));
    backgroundImageAct->setCheckable(true);
    backgroundImageAct->setChecked(false);
    connect(backgroundImageAct, SIGNAL(toggled(bool)), this, SLOT(slotBackgroundImage(bool)));



    /**
    Help menu actions
    */
    helpApp = new QAction(QIcon(":/images/help.png"), tr("Manual"),	this);
    helpApp ->setShortcut(tr("F1"));
    helpApp->setStatusTip(tr("Read the manual..."));
    helpApp->setWhatsThis(tr("Manual\n\nDisplays the documentation of SocNetV"));
    connect(helpApp, SIGNAL(triggered()), this, SLOT(slotHelp()));

    tipsApp = new QAction(tr("Tip of the Day"), this);
    tipsApp->setStatusTip(tr("Read useful tips"));
    tipsApp->setWhatsThis(tr("Quick Tips\n\nDisplays some useful and quick tips"));
    connect(tipsApp, SIGNAL(triggered()), this, SLOT(slotTips()));

    helpAboutApp = new QAction(tr("About SocNetV"), this);
    helpAboutApp->setStatusTip(tr("About SocNetV"));
    helpAboutApp->setWhatsThis(tr("About\n\nBasic information about SocNetV"));
    connect(helpAboutApp, SIGNAL(triggered()), this, SLOT(slotHelpAbout()));



    helpAboutQt = new QAction(tr("About Qt"), this);
    helpAboutQt->setStatusTip(tr("About Qt"));
    helpAboutQt->setWhatsThis(tr("About\n\nAbout Qt"));
    connect(helpAboutQt, SIGNAL(triggered()), this, SLOT(slotAboutQt() ) );
}



/**
  Creates and populates the MenuBar
*/
void MainWindow::initMenuBar() {


    /** menuBar entry networkMenu */
    networkMenu = menuBar()->addMenu(tr("&Network"));
    networkMenu -> addAction(fileNew);
    networkMenu -> addAction(fileOpen);
    importSubMenu = new QMenu(tr("Import ..."));
    importSubMenu -> addAction(importPajek);
    importSubMenu -> addAction(importSM);
    importSubMenu -> addAction(importTwoModeSM);
    importSubMenu -> addAction(importList);
    importSubMenu -> addAction(importDL);
    importSubMenu -> addAction(importDot);
    networkMenu ->addMenu (importSubMenu);

    networkMenu -> addSeparator();
    networkMenu -> addAction (openTextEditorAct);
    networkMenu -> addAction (viewNetworkFileAct);
    networkMenu -> addSeparator();
    networkMenu -> addAction (viewSociomatrixAct);
    networkMenu -> addSeparator();

    networkMenu -> addAction (recreateDataSetAct);
    networkMenu -> addSeparator();

    randomNetworkMenu = new QMenu(tr("Create Random Network..."));
    networkMenu ->addMenu (randomNetworkMenu);
    randomNetworkMenu -> addAction (createSmallWorldRandomNetworkAct);
    randomNetworkMenu -> addAction (createUniformRandomNetworkAct );
    // createGaussianRandomNetworkAct -> addTo(randomNetworkMenu);
    randomNetworkMenu -> addAction (createLatticeNetworkAct);
    randomNetworkMenu -> addAction (createSameDegreeRandomNetworkAct);
    networkMenu->addSeparator();

    networkMenu  -> addAction(webCrawlerAct);

    networkMenu  -> addSeparator();
    networkMenu  -> addAction(fileSave);
    networkMenu  -> addAction(fileSaveAs);
    networkMenu  -> addSeparator();

    exportSubMenu = networkMenu  -> addMenu(tr("Export..."));


    exportSubMenu -> addAction (exportBMP);
    exportSubMenu -> addAction (exportPNG);
    exportSubMenu -> addAction (exportPDF);
    exportSubMenu -> addSeparator();
    exportSubMenu -> addAction (exportSM);
    exportSubMenu -> addAction (exportPajek);
    //   exportList->addTo(exportSubMenu);
    //   exportDL->addTo(exportSubMenu);
    //   exportGW->addTo(exportSubMenu);

    networkMenu  -> addSeparator();
    networkMenu  -> addAction(printNetwork);
    networkMenu  -> addSeparator();
    networkMenu  -> addAction(fileClose);
    networkMenu  -> addAction(fileQuit);




    /** menuBar entry editMenu */

    editMenu = menuBar()->addMenu(tr("&Edit"));
    editNodeMenu = new QMenu(tr("Node..."));
    editNodeMenu -> setIcon(QIcon(":/images/node.png"));
    editMenu -> addMenu ( editNodeMenu );
    editNodeMenu -> addAction (findNodeAct);
    editNodeMenu -> addAction (addNodeAct);
    editNodeMenu -> addAction (removeNodeAct);

    editNodeMenu -> addSeparator();
    editNodeMenu -> addAction (changeNodeLabelAct);
    editNodeMenu -> addAction (changeNodeColorAct);
    editNodeMenu -> addAction (changeNodeSizeAct);
    editNodeMenu -> addAction (changeNodeValueAct);

    editNodeMenu -> addSeparator();
    editNodeMenu -> addAction (changeAllNodesSizeAct);
    editNodeMenu -> addAction (changeAllNodesShapeAct);
    editNodeMenu -> addAction (changeNumbersSizeAct);
    editNodeMenu -> addAction (changeLabelsSizeAct);

    editLinkMenu = new QMenu(tr("Link..."));
    editLinkMenu -> setIcon(QIcon(":/images/line.png"));
    editMenu-> addMenu (editLinkMenu);
    editLinkMenu -> addAction(addLinkAct);
    editLinkMenu -> addAction(removeLinkAct);
    editLinkMenu -> addAction(changeLinkLabelAct);
    editLinkMenu -> addAction(changeLinkColorAct);
    editLinkMenu -> addAction(changeLinkWeightAct);

    editMenu ->addSeparator();
    filterMenu = new QMenu ( tr("Filter..."));
    editMenu ->addMenu(filterMenu);

    filterMenu -> addAction(filterNodesAct );
    filterMenu -> addAction(filterOrphanNodesAct );
    filterMenu -> addAction(filterEdgesAct );

    editMenu ->addSeparator();
    //   transformNodes2LinksAct -> addTo (editMenu);
    editMenu -> addAction (symmetrizeAct);

    editNodeMenu -> addSeparator();
    colorOptionsMenu=new QMenu(tr("Colors"));
    colorOptionsMenu -> setIcon(QIcon(":/images/colorize.png"));
    editMenu -> addMenu (colorOptionsMenu);
    colorOptionsMenu -> addAction (changeBackColorAct);
    colorOptionsMenu -> addAction (changeAllNodesColorAct);
    colorOptionsMenu -> addAction (changeAllLinksColorAct);
    colorOptionsMenu -> addAction (changeAllNumbersColorAct);
    colorOptionsMenu -> addAction (changeAllLabelsColorAct);


    /** menuBar entry layoutMenu  */

    layoutMenu = menuBar()->addMenu(tr("&Layout"));
    //   colorationMenu = new QPopupMenu();
    //   layoutMenu -> insertItem (tr("Colorization"), colorationMenu);
    //   strongColorationAct -> addTo(colorationMenu);
    //   regularColorationAct-> addTo(colorationMenu);
    //   layoutMenu->insertSeparator();
    randomLayoutMenu = new QMenu(tr("Random..."));
    layoutMenu -> addMenu (randomLayoutMenu );
    randomLayoutMenu ->  addAction(randLayoutAct);
    randomLayoutMenu ->  addAction( randCircleLayoutAct );
    layoutMenu->addSeparator();

    circleLayoutMenu = new QMenu(tr("Radial by prominence index..."));
    circleLayoutMenu -> setIcon(QIcon(":/images/circular.png"));
    layoutMenu -> addMenu (circleLayoutMenu);
    circleLayoutMenu -> addAction (layoutRadial_DC_Act);
    circleLayoutMenu -> addAction (layoutRadial_CC_Act);
    circleLayoutMenu -> addAction (layoutRadial_IRCC_Act);
    circleLayoutMenu -> addAction (layoutRadial_BC_Act);
    circleLayoutMenu -> addAction (layoutRadial_SC_Act);
    circleLayoutMenu -> addAction (layoutRadial_EC_Act);
    circleLayoutMenu -> addAction (layoutRadial_PC_Act);
    circleLayoutMenu -> addAction (layoutRadial_IC_Act);
    circleLayoutMenu -> addAction (layoutRadial_DP_Act);
    circleLayoutMenu -> addAction (layoutRadial_PRP_Act);

    levelLayoutMenu = new QMenu (tr("Vertically Layered by prominence index..."));
    levelLayoutMenu -> setIcon(QIcon(":/images/net3.png"));
    layoutMenu -> addMenu (levelLayoutMenu);
    levelLayoutMenu ->addAction ( levelOutDegreeLayoutAct    );
    levelLayoutMenu ->addAction ( levelClosenessLayoutAct    );
    levelLayoutMenu ->addAction ( levelBetweenessLayoutAct   );
    levelLayoutMenu ->addAction ( levelInformationLayoutAct  );
    levelLayoutMenu ->addAction ( levelInDegreeLayoutAct     );

    layoutMenu->addSeparator();
    physicalLayoutMenu = new QMenu (tr("Physical..."));
    layoutMenu -> addMenu (physicalLayoutMenu);
    physicalLayoutMenu -> addAction (springLayoutAct);
    physicalLayoutMenu -> addAction (FRLayoutAct);
    layoutMenu->addSeparator();
    layoutMenu->addAction(nodeSizeProportionalOutDegreeAct);
    layoutMenu->addAction(nodeSizeProportionalInDegreeAct);
    layoutMenu->addSeparator();
    layoutMenu -> addAction (clearGuidesAct);



    /** menuBar entry: statistics menu */
    statMenu = menuBar()->addMenu(tr("&Statistics"));
    statMenu -> addAction (symmetryAct);
    statMenu -> addAction (invertAdjMatrixAct);
    //	statMenu -> addAction (netDensity);

    statMenu -> addSeparator();
    statMenu -> addAction (graphDistanceAct);
    statMenu -> addAction (averGraphDistanceAct);

    statMenu -> addAction (distanceMatrixAct);
    statMenu -> addAction (geodesicsMatrixAct);
    statMenu -> addAction (eccentricityAct);
    statMenu -> addAction (diameterAct);


    statMenu -> addSeparator();
    statMenu -> addAction(connectednessAct);
    statMenu -> addAction (walksAct);
    statMenu -> addAction (totalWalksAct);
    statMenu -> addAction (reachabilityMatrixAct);

    statMenu -> addSeparator();
    statMenu -> addAction (cliquesAct);
    statMenu -> addAction (clusteringCoefAct);

    statMenu -> addSeparator();
    statMenu -> addAction (triadCensusAct);

    statMenu->addSeparator();
    centrlMenu = new QMenu(tr("Centrality and Prestige indices..."));
    centrlMenu -> setIcon(QIcon(":/images/centrality.png"));
    statMenu->addMenu(centrlMenu);
    centrlMenu -> addSection(QIcon(":/images/centrality.png"), tr("Centrality"));

    centrlMenu -> addAction (cDegreeAct);
    centrlMenu -> addAction (cClosenessAct);
    centrlMenu -> addAction (cInfluenceRangeClosenessAct);
    centrlMenu -> addAction (cBetweenessAct);
    centrlMenu -> addAction (cStressAct);
    centrlMenu -> addAction (cEccentAct);
    centrlMenu -> addAction (cPowerAct);
    centrlMenu -> addAction (cInformationAct);
    centrlMenu -> addSection(QIcon(":/images/prestige.png"), tr("Prestige"));
    centrlMenu -> addAction (cInDegreeAct);
    centrlMenu -> addAction (cPageRankAct);
    centrlMenu -> addAction (cProximityPrestigeAct);

    /** menuBar entry optionsMenu  */
    optionsMenu = menuBar()->addMenu(tr("&Options"));
    nodeOptionsMenu=new QMenu(tr("Nodes..."));
    nodeOptionsMenu -> setIcon(QIcon(":/images/nodes.png"));

    optionsMenu -> addMenu (nodeOptionsMenu);
    nodeOptionsMenu -> addAction (displayNodeNumbersAct);
    nodeOptionsMenu -> addAction (displayNodeLabelsAct);
    nodeOptionsMenu -> addAction (displayNumbersInsideNodesAct);

    linkOptionsMenu=new QMenu(tr("Links..."));
    linkOptionsMenu -> setIcon(QIcon(":/images/line.png"));

    optionsMenu -> addMenu (linkOptionsMenu);
    linkOptionsMenu -> addAction (displayLinksAct);
    linkOptionsMenu -> addAction (displayLinksWeightNumbersAct);
    linkOptionsMenu -> addAction (displayLinksArrowsAct );
    linkOptionsMenu -> addSeparator();
    linkOptionsMenu -> addAction (drawLinksWeightsAct);
    linkOptionsMenu -> addAction (drawLinksBezier);

    viewOptionsMenu = new QMenu (tr("&View..."));
    viewOptionsMenu -> setIcon(QIcon(":/images/view.png"));
    optionsMenu -> addMenu (viewOptionsMenu);
    viewOptionsMenu-> addAction (backgroundImageAct);
    viewOptionsMenu-> addAction (antialiasingAct);
    viewOptionsMenu-> addAction (printDebugAct);
    viewOptionsMenu-> addAction (showProgressBarAct);
    viewOptionsMenu-> addAction (viewToolBar);
    viewOptionsMenu-> addAction (viewStatusBar);


    /**  menuBar entry helpMenu */
    helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu -> addAction (helpApp);
    helpMenu -> addAction (tipsApp);
    helpMenu -> addSeparator();
    helpMenu-> addAction (helpAboutApp);
    helpMenu-> addAction (helpAboutQt);


}



/**
    Initializes the toolbar
*/
void MainWindow::initToolBar(){
    toolBar = addToolBar("operations");
    toolBar -> addAction (fileNew);
    toolBar -> addAction (fileOpen);
    toolBar -> addAction (fileSave);
    toolBar -> addAction (printNetwork);
    toolBar -> addSeparator();

    toolBar -> addAction(zoomInAct);

    //Create zooming widget
    zoomCombo = new QComboBox;
    QStringList scales;
    scales << tr("25%") << tr("50%") << tr("75%") << tr("100%") << tr("125%") << tr("150%")<<tr("175%")  ;
    zoomCombo->addItems(scales);
    zoomCombo->setCurrentIndex(3);

    toolBar -> addWidget(zoomCombo);
    toolBar -> addAction(zoomOutAct);

    toolBar -> addSeparator();

    QLabel *labelRotateSpinBox= new QLabel;
    labelRotateSpinBox ->setText(tr("Rotation:"));


    rotateSpinBox = new QSpinBox;
    rotateSpinBox ->setRange(-360, 360);
    rotateSpinBox->setSingleStep(1);
    rotateSpinBox->setValue(0);

    QGroupBox *rotateGroup = new QGroupBox();
    QHBoxLayout *rotateGroupLayout = new QHBoxLayout(rotateGroup);
    rotateGroupLayout->addWidget(labelRotateSpinBox);
    rotateGroupLayout->addWidget(rotateSpinBox);

    toolBar -> addWidget(rotateGroup);

    toolBar -> addSeparator();
    toolBar -> addAction ( QWhatsThis::createAction (this));


}







//Creates a dock widget for instant menu access
void MainWindow::initToolBox(){
    toolBox = new QTabWidget;
    toolBox->setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Ignored));


    //create widgets for the buttons group/tab
    addNodeBt= new QPushButton(QIcon(":/images/add.png"),tr("&Add Node"));
    addNodeBt->setFocusPolicy(Qt::NoFocus);
    addNodeBt->setToolTip(tr("Add a new node to the network (Ctrl+A). \n\n Alternately, you can create a new node \nin a specific position by double-clicking \non that spot of the canvas."));
    removeNodeBt= new QPushButton(QIcon(":/images/remove.png"),tr("&Remove Node"));
    removeNodeBt->setFocusPolicy(Qt::NoFocus);
    removeNodeBt->setToolTip(tr("Remove a node from the network (Ctrl+Shift+A). \n\n Alternately, you can remove a node \nby right-clicking on it."));

    addLinkBt= new QPushButton(QIcon(":/images/connect.png"),tr("Add &Link"));
    addLinkBt->setFocusPolicy(Qt::NoFocus);
    addLinkBt->setToolTip(tr("Add a new link to the network (Ctrl+L).\n\n Alternately, you can create a new link between two \nnodes by middle-clicking on them consequetively."));

    removeLinkBt= new QPushButton(QIcon(":/images/disconnect.png"),tr("Remove Link"));
    removeLinkBt->setFocusPolicy(Qt::NoFocus);
    removeLinkBt->setToolTip(tr("Remove a link from the network  \n\n Alternately, you can remove a link \nby right-clicking on it."));

    //create a layout for these widgets
    QGridLayout *buttonsGrid = new QGridLayout;
    buttonsGrid -> addWidget(addNodeBt, 0,0);
    buttonsGrid -> addWidget(removeNodeBt, 0,1);
    buttonsGrid -> addWidget(addLinkBt,1,0);
    buttonsGrid -> addWidget(removeLinkBt,1,1);
    buttonsGrid -> setRowStretch(2,1);   //fix vertical stretch
    //create a box with a title & a frame. Inside, display the vertical layout of widgets
    QGroupBox *buttonsGroup= new QGroupBox(tr(""));
    buttonsGrid->setSpacing(0);
    buttonsGrid->setMargin(10);
    buttonsGroup->setLayout(buttonsGrid);

    toolBox->addTab(buttonsGroup, tr("Edit"));


    //create widgets for Properties/Statistics group/tab
    QLabel *labelNodesLCD = new QLabel;
    labelNodesLCD->setText(tr("Total Nodes"));
    QLabel *labelEdgesLCD = new QLabel;
    labelEdgesLCD->setText(tr("Total Links"));
    nodesLCD=new QLCDNumber(7);
    nodesLCD->setSegmentStyle(QLCDNumber::Flat);
    nodesLCD->setToolTip(tr("Counts how many nodes (vertices) exist in the whole network."));
    edgesLCD=new QLCDNumber(7);
    edgesLCD->setSegmentStyle(QLCDNumber::Flat);
    edgesLCD->setToolTip(tr("Counts how many links (in and out) exist in the whole network."));

    QLabel *labelDensityLCD = new QLabel;
    labelDensityLCD->setText(tr("Density"));
    densityLCD=new QLCDNumber(7);
    densityLCD->setSegmentStyle(QLCDNumber::Flat);
    densityLCD->setToolTip(tr("The density of a network is the ratio of existing links to all possible links (n(n-1)) between nodes."));

    //create a grid layout
    QGridLayout *propertiesGrid = new QGridLayout();

    propertiesGrid -> setColumnMinimumWidth(0, 10);
    propertiesGrid -> setColumnMinimumWidth(1, 10);

    propertiesGrid -> addWidget(labelNodesLCD, 0,0);
    propertiesGrid -> addWidget(labelEdgesLCD, 0,1);
    propertiesGrid -> addWidget(nodesLCD,1,0);
    propertiesGrid -> addWidget(edgesLCD,1,1);

    propertiesGrid -> addWidget(labelDensityLCD, 2,0);
    propertiesGrid -> addWidget(densityLCD,2,1);

    QLabel *dummyLabel = new QLabel;
    dummyLabel-> setText (" ");
    QLabel *labelNode = new QLabel;
    labelNode-> setText (tr("Active Node"));

    QLabel *labelSelectedNodeLCD = new QLabel;
    labelSelectedNodeLCD -> setText (tr("Node Number:"));
    labelSelectedNodeLCD -> setToolTip (tr("This is the number of the last selected node."));

    selectedNodeLCD =new QLCDNumber(7);
    selectedNodeLCD ->setSegmentStyle(QLCDNumber::Flat);

    QLabel *labelInLinksLCD = new QLabel;
    labelInLinksLCD -> setText (tr("Node In-Degree:"));
    labelInLinksLCD -> setToolTip (tr("The sum of all in-edge weights of the node you clicked.."));
    inLinksLCD=new QLCDNumber(7);
    inLinksLCD -> setSegmentStyle(QLCDNumber::Flat);
    inLinksLCD -> setToolTip (tr("The sum of all in-edge weights of the node you clicked."));
    QLabel *labelOutLinksLCD = new QLabel;
    labelOutLinksLCD -> setText (tr("Node Out-Degree:"));
    labelOutLinksLCD -> setToolTip (tr("The sum of all out-edge weights of the node you clicked."));
    outLinksLCD=new QLCDNumber(7);
    outLinksLCD -> setSegmentStyle(QLCDNumber::Flat);
    outLinksLCD -> setToolTip (tr("The sum of all out-edge weights of the node you clicked."));

    QLabel *labelClucofLCD  = new QLabel;
    labelClucofLCD -> setText (tr("Clustering Coef."));
    labelClucofLCD -> setToolTip (tr("The Clustering Coefficient quantifies how close the clicked vertex and its neighbors are to being a clique. \nThe value is the proportion of links between the vertices within the neighbourhood of the clicked vertex,\n divided by the number of links that could possibly exist between them. \n\n WARNING: This value is displayed for each node when you click on it,\n but only if you have computed CluCof from the menu Statistics > Clustering Coefficient "));
    clucofLCD = new QLCDNumber(7);
    clucofLCD -> setSegmentStyle(QLCDNumber::Flat);
    clucofLCD  -> setToolTip (tr("The Clustering Coefficient quantifies how close the clicked vertex and its neighbors are to being a clique. \nThe value is the proportion of links between the vertices within the neighbourhood of the clicked vertex,\n divided by the number of links that could possibly exist between them. \n\n WARNING: This value is displayed for each node when you click on it,\n but only if you have computed CluCof from the menu Statistics > Clustering Coefficient "));


    propertiesGrid -> addWidget(dummyLabel, 6,0);
    propertiesGrid -> addWidget(labelNode, 7,0);
    propertiesGrid -> addWidget(labelSelectedNodeLCD , 8,0);
    propertiesGrid -> addWidget(selectedNodeLCD ,8,1);
    propertiesGrid -> addWidget(labelInLinksLCD, 9,0);
    propertiesGrid -> addWidget(inLinksLCD, 9,1);
    propertiesGrid -> addWidget(labelOutLinksLCD, 10,0);
    propertiesGrid -> addWidget(outLinksLCD,10,1);
    propertiesGrid -> addWidget(labelClucofLCD, 11,0);
    propertiesGrid -> addWidget(clucofLCD,11,1);
    propertiesGrid -> setRowStretch(12,1);   //fix stretch

    //create a box with title
    QGroupBox *networkPropertiesGroup = new QGroupBox(tr(""));
    networkPropertiesGroup -> setLayout (propertiesGrid);


    toolBox->addTab( networkPropertiesGroup, tr("Statistics"));
    toolBox->setMinimumWidth(buttonsGroup->sizeHint().width());



    // create some more widgets for the final tab: "Layout"
    moveSpringEmbedderBx = new QCheckBox(tr("Spring Embedder") );
    moveSpringEmbedderBx->setEnabled(true);
    moveSpringEmbedderBx->setChecked(false);
    moveSpringEmbedderBx->setToolTip(tr("Embeds a spring-gravitational model on the network, where \neach node is regarded as physical object reppeling all \nother nodes, while springs between connected nodes attact them. \nThe result is \nconstant movement. This is a very SLOW process on networks with N > 100!"));

    moveFruchtermanBx = new QCheckBox(tr("Fruchterman-Reingold") );
    moveFruchtermanBx->setEnabled(false);
    moveFruchtermanBx->setToolTip(tr("In Fruchterman-Reingold model, the vertices behave as atomic particles or celestial bodies, exerting attractive and repulsive forces to each other. Again, only vertices that are neighbours attract each other but, unlike Spring Embedder, all vertices repel each other. "));

    moveKamandaBx= new QCheckBox(tr("Kamanda-Kwei") );
    moveKamandaBx->setEnabled(false);
    moveKamandaBx->setToolTip(tr("!"));


    nodeSizeProportional2OutDegreeBx = new QCheckBox(tr("Node sizes follow OutDegree)") );
    nodeSizeProportional2OutDegreeBx ->setEnabled(true);
    nodeSizeProportional2OutDegreeBx->setToolTip(tr("If you enable this, all nodes will be resized so that their size reflect their out-degree (the amount of links from them). To put it simply, more out-linked nodes will be bigger..."));

    nodeSizeProportional2InDegreeBx = new QCheckBox(tr("Node sizes follow InDegree") );
    nodeSizeProportional2InDegreeBx ->setEnabled(true);
    nodeSizeProportional2InDegreeBx->setToolTip(tr("If you enable this, all nodes will be resized so that their size reflect their in-degree (the amount of links to them from other nodes). To put it simply, more in-linked nodes will be bigger..."));

    QGridLayout *layoutGroupLayout = new QGridLayout();
    layoutGroupLayout -> addWidget(moveSpringEmbedderBx, 0,0);
    layoutGroupLayout -> addWidget(moveKamandaBx, 1,0);
    layoutGroupLayout -> addWidget(nodeSizeProportional2OutDegreeBx, 2,0);
    layoutGroupLayout -> addWidget(nodeSizeProportional2InDegreeBx, 3,0);

    layoutGroupLayout -> setRowStretch(4,1);   //fix stretch

    //create a box with title
    QGroupBox *layoutGroup= new QGroupBox(tr(""));
    layoutGroup->setLayout (layoutGroupLayout );
    toolBox->addTab(layoutGroup, tr("Layout"));


    connect(moveSpringEmbedderBx, SIGNAL(clicked(bool)),this, SLOT(slotLayoutSpringEmbedder(bool)));
    connect(moveFruchtermanBx, SIGNAL(stateChanged(int)),this, SLOT(layoutFruchterman(int)));

    connect(nodeSizeProportional2OutDegreeBx , SIGNAL(clicked(bool)),this, SLOT(slotLayoutNodeSizeProportionalOutEdges(bool)));
    connect(nodeSizeProportional2InDegreeBx , SIGNAL(clicked(bool)),this, SLOT(slotLayoutNodeSizeProportionalInEdges(bool)));

}









//FIXME this is a bug: Graph calls GraphicsWidget which calls this to call Graph!
void MainWindow::updateNodeCoords(int nodeNumber, int x, int y){
    //	qDebug("MW: updateNodeCoords() for %i with x %i and y %i", nodeNumber, x, y);
    activeGraph.updateVertCoords(nodeNumber, x, y);
}




/**
    Initializes the status bar
*/
void MainWindow::initStatusBar() {
    statusBarDuration=3000;
    statusMessage( tr("Ready."));
}







/**
    Initializes the scene and its graphicsWidget, the main widget of SocNetV
*/
void MainWindow::initView() {
    qDebug ("MW initView()");
    //create a scene
    scene=new QGraphicsScene();

    //create a view widget for this scene
    graphicsWidget=new GraphicsWidget(scene, this);
    graphicsWidget->setViewportUpdateMode( QGraphicsView::BoundingRectViewportUpdate );
    //  FullViewportUpdate  // MinimalViewportUpdate //SmartViewportUpdate  //BoundingRectViewportUpdate
    //QGraphicsView can cache pre-rendered content in a QPixmap, which is then drawn onto the viewport.
    graphicsWidget->setCacheMode(QGraphicsView::CacheNone);  //CacheBackground | CacheNone

    graphicsWidget->setRenderHint(QPainter::Antialiasing, true);
    graphicsWidget->setRenderHint(QPainter::TextAntialiasing, true);
    graphicsWidget->setRenderHint(QPainter::SmoothPixmapTransform, true);
    //Optimization flags:
    //if items do restore their state, it's not needed for graphicsWidget to do the same...
    graphicsWidget->setOptimizationFlag(QGraphicsView::DontSavePainterState, false);
    //Disables QGraphicsView's antialiasing auto-adjustment of exposed areas.
    graphicsWidget->setOptimizationFlag(QGraphicsView::DontAdjustForAntialiasing, false);
    //"QGraphicsScene applies an indexing algorithm to the scene, to speed up item discovery functions like items() and itemAt().
    // Indexing is most efficient for static scenes (i.e., where items don't move around).
    // For dynamic scenes, or scenes with many animated items, the index bookkeeping can outweight the fast lookup speeds." So...
    scene->setItemIndexMethod(QGraphicsScene::BspTreeIndex); //NoIndex (for anime) | BspTreeIndex

    graphicsWidget->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    graphicsWidget->setResizeAnchor(QGraphicsView::AnchorViewCenter);

    // sets dragging the mouse over the scene while the left mouse button is pressed.
    graphicsWidget->setDragMode(QGraphicsView::RubberBandDrag);
    graphicsWidget->setFocusPolicy(Qt::StrongFocus);
    graphicsWidget->setFocus();

    this->resize(900,600);

    //set minimum size of canvas

    graphicsWidget->setMinimumSize( (qreal)  ( this->width()-toolBox->sizeHint().width() -40 ) , (qreal) ( this->height()-statusBar()->sizeHint().height() -toolBar->sizeHint().height() -menuBar()->sizeHint().height() -20 ) );
    qDebug ("MW initView(): now window size %i, %i, graphicsWidget size %i, %i, scene %f,%f",this->width(),this->height(), graphicsWidget->width(),graphicsWidget->height(), graphicsWidget->scene()->width(), graphicsWidget->scene()->height());

}




/**
    Resizes the scene when the window is resized.
*/
void MainWindow::resizeEvent( QResizeEvent * ){
    qDebug ("MW resizeEvent():INITIAL window size %i, %i, graphicsWidget size %i, %i, scene %f,%f",this->width(),this->height(), graphicsWidget->width(),graphicsWidget->height(), graphicsWidget->scene()->width(), graphicsWidget->scene()->height());

    //the area of the scene displayed by the CanvasView
    scene->setSceneRect(0, 0, (qreal) ( graphicsWidget->width() -5 ), (qreal) (graphicsWidget->height() -5 ) );
    qDebug ("MW resizeEvent(): now window size %i, %i, graphicsWidget size %i, %i, scene %f,%f",this->width(),this->height(), graphicsWidget->width(),graphicsWidget->height(), graphicsWidget->scene()->width(), graphicsWidget->scene()->height());
}


/**
    Initializes the default network parameters.
    Also used when erasing a network to start a new one
*/
void MainWindow::initNet(){
    qDebug()<<"MW: initNet() START INITIALISATION";
    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

    // Init basic variables

    initNodeSize=4;
    initNodeColor="red";
    initLinkColor="black";
    initLabelColor="darkblue";
    initLabelSize=7;
    initNumberSize=7;
    initNumberColor="black";
    initNodeShape="circle";
    initBackgroundColor="white"; //"gainsboro";

    minDuration=3000; //dialogue duration - obsolete
    maxNodes=5000;		//Max nodes used by createRandomNetwork dialogues
    labelDistance=8;
    numberDistance=5;
    totalLinks=0;
    networkName="";

    previous_fileName=fileName;
    fileName="";

    pajekFileLoaded=false;
    adjacencyFileLoaded=false;
    fileFormat = -1;
    dotFileLoaded=false;
    fileLoaded=false;

    networkModified=false;
    fileSave->setIcon(QIcon(":/images/saved.png"));
    fileSave->setEnabled(true);

    markedNodeExists=false;	//used by slotFindNode()

    cursorPosGW=QPointF(-1,-1);
    clickedJimNumber=-1;
    linkClicked=false;
    nodeClicked=false;

    /** Clear previous network data */
    activeGraph.clear();
    activeGraph.setSocNetV_Version(VERSION);

    activeGraph.setInitVertexShape(initNodeShape);
    activeGraph.setInitVertexSize(initNodeSize);
    activeGraph.setInitVertexColor(initNodeColor);

    activeGraph.setInitVertexNumberSize(initNumberSize);
    activeGraph.setInitVertexNumberColor(initNumberColor);

    activeGraph.setInitVertexLabelColor(initLabelColor);
    activeGraph.setInitVertexLabelSize(initLabelSize);

    activeGraph.setInitEdgeColor(initLinkColor);

    activeGraph.setShowLabels(this->showLabels());
    activeGraph.setShowNumbersInsideNodes( this->showNumbersInsideNodes());


    /** Clear scene **/
    graphicsWidget->clear();


    /** Clear LCDs **/
    nodesLCD->display(activeGraph.vertices());
    edgesLCD->display(activeGraph.totalEdges());
    densityLCD->display(activeGraph.density());
    inLinksLCD->display(0);
    outLinksLCD->display(0);
    clucofLCD->display(0);
    selectedNodeLCD->display(0);

    /** Clear toolbox and menu checkboxes **/
    nodeSizeProportional2OutDegreeBx->setChecked(false);
    nodeSizeProportional2InDegreeBx->setChecked(false);
    moveSpringEmbedderBx->setChecked(false);
    springLayoutAct->setChecked(false);
    FRLayoutAct->setChecked(false);
    displayLinksWeightNumbersAct->setChecked(false);
    //displayLinksArrowsAct->setChecked(false);		//FIXME: USER PREFS EMITTED TO GRAPH?

    filterOrphanNodesAct->setChecked(false); // re-init orphan nodes menu item

    /** set window title **/
    setWindowTitle(tr("Social Network Visualizer ")+VERSION);

    QApplication::restoreOverrideCursor();
    statusMessage( tr("Ready"));
    qDebug("MW: initNet() INITIALISATION END");
}




/*
 * Slot called by Graph::statusMessage to display some message to the user
 */
void MainWindow::statusMessage(const QString message){
    statusBar()->showMessage( message, statusBarDuration );
}


/**
*	Displays a message	on the status bar when you resize the window.
*/
void MainWindow::windowInfoStatusBar(int w, int h){
    statusMessage(  QString(tr("Window resized to (%1, %2) pixels.")).arg(w).arg(h) );
}



/**
    Closes the application.
    Asks to write any unsaved network data.
*/
void MainWindow::closeEvent( QCloseEvent* ce ) {
    if ( !networkModified )       {
        ce->accept();
        return;
    }
    switch( QMessageBox::information( this, "Save file",
                                      tr("Do you want to save the changes") +
                                      tr(" to the network file?"),
                                      tr("Yes"), tr("No"), tr("Cancel"),
                                      0, 1 ) )
    {
    case 0:
        slotFileSave();
        ce->accept();
        break;
    case 1:
        ce->accept();
        break;
    case 2:
    default: // just for sanity
        ce->ignore();
        break;
    }
}




/**
    Creates a new network
*/
void MainWindow::slotCreateNew() {
    slotFileClose();
}




/**
    Prompts the user a directory dialogue to choose a file from.
    Loads the specified file, calling loadNetworkFile()
*/
void MainWindow::slotChooseFile() {

    if (firstTime && fileFormat == -1 ) {
        QMessageBox::information( this, "SocNetV",
                                  tr("Attention: \n")+
                                  tr("This menu option is suitable only for loading a network file with data in GraphML format, which is the default file format of SocNetV. \n")+

                                  tr("If you want to import other supported network formats (i.e. Pajek, UCINET, dot, etc), ")+
                                  tr("please use the options in the Import sub menu. \n")+
                                  tr("\n This warning message will not appear again."),
                                  "OK", 0 );
        firstTime=false;
    }
    if ( fileFormat == -1 )
        fileFormat = 1;

    bool a_file_was_already_loaded=fileLoaded;
    previous_fileName=fileName;
    QString m_fileName, fileType_string;
    int m_fileFormat=fileFormat;

    statusMessage( tr("Choose a network file..."));
    switch (m_fileFormat){
    case 1:	//GraphML
        fileType_string = tr("GraphML (*.graphml *.xml);;All (*)");
        break;
    case 2: //Pajek
        fileType_string = tr("Pajek (*.net *.paj *.pajek);;All (*)");
        break;
    case 3: //Adjacency
        fileType_string = tr("Adjacency (*.txt *.csv *.sm *.adj);;All (*)");
        break;
    case 4: //Dot
        fileType_string = tr("GraphViz (*.dot);;All (*)");
        break;
    case 5:	//GML
        fileType_string = tr("GML (*.gml);;All (*)");
        break;
    case 6: //DL
        fileType_string = tr("DL (*.dl);;All (*)");
        break;
    case 7:	// Weighted List
        fileType_string = tr("List (*.lst *.list);;All (*)");
        break;
    case 8:	// Simple List
        fileType_string = tr("List (*.lst *.list);;All (*)");
        break;
    case 9:	// Two mode sm
        fileType_string = tr("Two-Mode Sociomatrix (*.txt *.2sm *.aff *.csv  *.sm);;All (*)");
        break;
    default:	//All
        fileType_string = tr("All (*);;GraphML (*.graphml);;GraphViz (*.dot);;Adjacency (*.txt *.csv *.net *.adj *.sm);;Pajek (*.net *.pajek *.paj);;DL (*.dl *.net)");
        break;

    }
    m_fileName = QFileDialog::getOpenFileName( this, tr("Select one file to open"), "", fileType_string	);

    if (!m_fileName.isEmpty()) {
        qDebug()<<"MW: file selected: " << m_fileName;
        fileNameNoPath=m_fileName.split ("/" );
        if ( loadNetworkFile ( m_fileName, m_fileFormat  ) )
        {
            fileName=m_fileName;
            previous_fileName=fileName;
            setWindowTitle("SocNetV "+ VERSION +" - "+fileNameNoPath.last());
            QString message=tr("Loaded network: ")+fileNameNoPath.last();
            statusMessage( message );
        }
        else {
            statusMessage( tr("Error loading requested file. Aborted."));
            QMessageBox::critical( this, "SocNetV",
                                   tr("Error! \n")+
                                   tr("Sorry, the selected file is not in GraphML format, which is the default file format of SocNetV. \n")+
                                   tr("If you want to import other network formats (i.e. Pajek, UCINET, dot, etc), ")+
                                   tr("please use the options in the Import sub menu. \n"),
                                   "OK", 0 );
        }
    }
    else  {
        statusMessage( tr("Opening aborted"));
        //if a file was previously opened, get back to it.
        if (a_file_was_already_loaded)	{
            fileLoaded=true;
            fileName=previous_fileName;
        }
    }
}





/**
    Saves the network in the same file
*/
void MainWindow::slotFileSave() {
    statusMessage( tr("Saving file..."));

    if (!fileLoaded && !networkModified ) {
        statusMessage(  QString(tr("No network loaded.")) );
        return;
    }
    if ( fileName.isEmpty() ) {
        slotFileSaveAs();
        return;
    }

    int maxWidth=scene->width();
    int maxHeight=scene->height();
    fileNameNoPath=fileName.split ("/");
    if (pajekFileLoaded)
    {
        if ( activeGraph.saveGraph(fileName, 1, networkName, maxWidth,maxHeight) )
            networkSaved(1);
        else
            networkSaved(0);
    }
    else if (adjacencyFileLoaded)
    {
        if ( activeGraph.saveGraph(fileName, 2, networkName, maxWidth,maxHeight) )
            networkSaved(2);
        else
            networkSaved(0);
    }
    else if (graphMLFileLoaded || ( !fileLoaded && networkModified) )
    {	//new file or GraphML
        if ( activeGraph.saveGraph(fileName, 4, networkName, maxWidth,maxHeight) )
            networkSaved(4);
        else
            networkSaved(0);
    }
    else
    {
        switch( QMessageBox::information( this, "GraphML File Format",
                                          tr("This network will be saved in GraphML format. \n")+
                                          tr("Is this OK? \n\n") +
                                          tr("If not, press Cancel, then go to Network > Export menu..."),
                                          "Yes", "No",0,1 )
                )
        {
        case 0:
            if ( activeGraph.saveGraph(fileName, 4, networkName, maxWidth,maxHeight) )
                networkSaved(4);
            else
                networkSaved(0);
            break;
        case 1:
            statusMessage( tr("Save aborted...") );
            break;
        }
    }

}



/**
    Saves the network in a new file
*/
void MainWindow::slotFileSaveAs() {
    statusMessage( tr("Saving network under new filename..."));

    QString fn =  QFileDialog::getSaveFileName(this,
                                               tr("Save GraphML Network to File Named..."),
                                               0, tr("GraphML (*.graphml *.xml);;All (*)") );
    if (!fn.isEmpty())  {
        if  ( QFileInfo(fn).suffix().isEmpty() ){
            QMessageBox::information(this, "Missing Extension ",tr("File extension was missing! \nI am appending a standard .graphml to the given filename."), "OK",0);
            fn.append(".graphml");
        }
        fileName=fn;
        fileNameNoPath=fileName.split ("/");
        adjacencyFileLoaded=false;
        pajekFileLoaded=false;
        graphMLFileLoaded=false;
        slotFileSave();
    }
    else  {
        statusMessage( tr("Saving aborted"));
        return;
    }
    statusMessage( tr("Ready."));
}



/*
 *	Called from Graph when we try to save file
 */
void MainWindow::networkSaved(int saved_ok)
{
    if (saved_ok <= 0)
    {
        graphChanged();
        statusMessage( tr("Error! Could not save this file... ")+fileNameNoPath.last()+tr(".") );
    }
    else
    {
        fileSave->setIcon(QIcon(":/images/saved.png"));
        fileSave->setEnabled(false);
        fileLoaded=true; networkModified=false;
        setWindowTitle( fileNameNoPath.last() );
        statusMessage( tr("Network saved under filename: ")+fileNameNoPath.last()+tr(".") );
        switch (saved_ok){
        case 1:
            adjacencyFileLoaded=false;
            pajekFileLoaded=true;
            graphMLFileLoaded=false;
            break;
        case 2:
            adjacencyFileLoaded=true;
            pajekFileLoaded=false;
            graphMLFileLoaded=false;
            break;
        case 3:
            adjacencyFileLoaded=false;
            pajekFileLoaded=false;
            graphMLFileLoaded=false;
            break;
        case 4:
            adjacencyFileLoaded=false;
            pajekFileLoaded=false;
            graphMLFileLoaded=true;
            break;
        }
    }
}

/**
    Closes the network. Saves it if necessary.
    Used by createNew.
*/
void MainWindow::slotFileClose() {
    statusMessage( tr("Closing file..."));
    qDebug()<<"slotFileClose()";
    if (networkModified) {
        switch ( QMessageBox::information (this,
                                           "Closing Network...",
                                           tr("Network has not been saved. \nDo you want to save before closing it?"),
                                           "Yes", "No",0,1))
        {
        case 0: slotFileSave(); break;
        case 1: break;
        }
    }
    statusMessage( tr("Erasing old network data...."));
    initNet();
    statusMessage( tr("Ready."));
}



/**
    Prints whatever is viewable on the Graphics widget
*/
void MainWindow::slotPrintView() {
    statusMessage( tr("Printing..."));
    QPrintDialog dialog(printer, this);
    if ( dialog.exec() )   {
        QPainter painter(printer);
        graphicsWidget->render(&painter);
    };
    statusMessage( tr("Ready."));
}




/**	
    Imports a network from a formatted file
*/
void MainWindow::slotImportGraphML(){
    fileFormat=1;
    this->slotChooseFile();
}



/**	
    Imports a network from a formatted file
*/
void MainWindow::slotImportPajek(){
    fileFormat=2;
    this->slotChooseFile();
}



/**	
    Imports a network from a Adjacency matrix formatted file
*/
void MainWindow::slotImportSM(){
    fileFormat=3;
    this->slotChooseFile();
}



/**
    Imports a network from a two mode sociomatrix formatted file
*/
void MainWindow::slotImportTwoModeSM(){
    fileFormat=9;
    this->slotChooseFile();
}


/**	
    Imports a network from a Dot formatted file
*/
void MainWindow::slotImportDot(){
    fileFormat=4;
    this->slotChooseFile();
}



/**	
    Imports a network from a GML formatted file
*/
void MainWindow::slotImportGML(){
    fileFormat=5;
    this->slotChooseFile();
}


/**	
    Imports a network from a UCINET formatted file
*/
void MainWindow::slotImportDL(){
    fileFormat=6;
    this->slotChooseFile();
}


/**	
    Imports a network from a List formatted file
*/
void MainWindow::slotImportEdgeList(){
    switch( QMessageBox::question( this, "Type of list format",
                                   tr("I can parse two kinds of lists: \n\n")+
                                   tr("A. Weighted lists, with each line having exactly 3 columns (source, target, weight), i.e.\n  1 2 5 \n \n")+
                                   tr("B. Simple edge lists, with each line having 2 or more columns (source, target1, target2, ... etc)\n\n")+
                                   tr("Please select the appropriate type of list format for the file you want to load:"),
                                   "Weighted", "Simple",0,1 )
            )
    {
    case 0:
        qDebug() << "***  MW: slotImportEdgeList - Weighted list selected! " ;
        fileFormat  = 7;
        break;
    case 1:
        qDebug() << "***  MW: slotImportEdgeList - Simple list selected! " ;
        fileFormat = 8;
        break;
    }
    this->slotChooseFile();
}




/**
 * 	Main network file loader method
 * 	First, inits everything to default values.
 *      Then calls activeGraph::loadGraph to actually load the network...
 */
bool MainWindow::loadNetworkFile(QString m_fileName, int m_fileFormat ){
    qDebug("MW: loadNetworkFile");
    initNet();
    int two_sm_mode = 0;

    if ( m_fileFormat == 9 ) {
        switch( QMessageBox::information( this, "Two-mode sociomatrix",
                                          tr("If this file is in two-mode sociomatrix format, \n")+
                                          tr("please specify which mode to open \n\n") +
                                          tr("1st mode: rows are nodes \n") +
                                          tr("2nd mode: columns are nodes"),
                                          tr("1st Mode"), tr("2nd mode"), 0,1 ) ) {
        case 0:
            two_sm_mode = 1;
            break;
        case 1:
            two_sm_mode = 2;
            break;
        }
    }
    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
    bool loadGraphStatus = activeGraph.loadGraph (
                m_fileName,
                displayNodeLabelsAct->isChecked(),
                graphicsWidget->width(),
                graphicsWidget->height(),
                m_fileFormat, two_sm_mode
                );
    QApplication::restoreOverrideCursor();
    return loadGraphStatus;
}


/**
*	Called from Parser/Graph when a network file is loaded.
*	It informs the MW about the type of the network so that it can display the appropiate message.
*/
void MainWindow::fileType (
        int type, QString netName, int aNodes, int totalLinks, bool undirected)
{
    qDebug()<< "MW: fileType() networkName is: " << netName << " type " << type;
    Q_UNUSED (undirected);
    if (netName != "")
        networkName=netName ;
    else
        networkName=(fileName.split ("/")).last();
    fileFormat=type;
    switch( type ) 	{
    case 0:
        pajekFileLoaded=false;
        adjacencyFileLoaded=false;
        graphMLFileLoaded=false;
        fileLoaded=false;
        break;
    case 1:
        pajekFileLoaded=false;
        adjacencyFileLoaded=false;
        dotFileLoaded=false;
        graphMLFileLoaded=true;
        fileLoaded=true;
        networkModified=false;
        statusMessage( QString(tr("GraphML formatted network, named %1, loaded with %2 Nodes and %3 total Links.")).arg( networkName ).arg( aNodes ).arg(totalLinks ) );
        break;

    case 2:
        pajekFileLoaded=true;
        adjacencyFileLoaded=false;
        graphMLFileLoaded=false;
        fileLoaded=true;
        networkModified=false;
        statusMessage( QString(tr("Pajek formatted network, named %1, loaded with %2 Nodes and %3 total Links.")).arg( networkName ).arg( aNodes ).arg(totalLinks ));
        break;

    case 3:
        pajekFileLoaded=false;
        adjacencyFileLoaded=true;
        graphMLFileLoaded=false;
        fileLoaded=true;
        networkModified=false;
        statusMessage( QString(tr("Adjacency formatted network, named %1, loaded with %2 Nodes and %3 total Links.")).arg( networkName ).arg( aNodes ).arg(totalLinks ) );
        break;

    case 4:
        pajekFileLoaded=false;
        adjacencyFileLoaded=false;
        dotFileLoaded=true;
        graphMLFileLoaded=false;
        fileLoaded=true;
        networkModified=false;
        statusMessage( QString(tr("Dot formatted network, named %1, loaded with %2 Nodes and %3 total Links.")).arg( networkName ).arg( aNodes ).arg(totalLinks ) );
        break;

    case 5:
        pajekFileLoaded=false;
        adjacencyFileLoaded=false;
        dotFileLoaded=false;
        graphMLFileLoaded=false;
        fileLoaded=true;
        networkModified=false;
        statusMessage( QString(tr("DL-formatted network, named %1, loaded with %2 Nodes and %3 total Links.")).arg( networkName ).arg( aNodes ).arg(totalLinks ) );
        break;
    case 6:
        pajekFileLoaded=false;
        adjacencyFileLoaded=false;
        dotFileLoaded=false;
        graphMLFileLoaded=false;
        fileLoaded=true;
        networkModified=false;
        statusMessage( QString(tr("GML-formatted network, named %1, loaded with %2 Nodes and %3 total Links.")).arg( networkName ).arg( aNodes ).arg(totalLinks ) );
        break;
    case 7:
        pajekFileLoaded=false;
        adjacencyFileLoaded=false;
        dotFileLoaded=false;
        graphMLFileLoaded=false;
        fileLoaded=true;
        networkModified=false;
        statusMessage( QString(tr("Weighted list-formatted network, named %1, loaded with %2 Nodes and %3 total Links.")).arg( networkName ).arg( aNodes ).arg(totalLinks ) );
        break;
    case 8:
        pajekFileLoaded=false;
        adjacencyFileLoaded=false;
        dotFileLoaded=false;
        graphMLFileLoaded=false;
        fileLoaded=true;
        networkModified=false;
        statusMessage( QString(tr("Simple list-formatted network, named %1, loaded with %2 Nodes and %3 total Links.")).arg( networkName ).arg( aNodes ).arg(totalLinks ) );
        break;
    case 9:
        pajekFileLoaded=false;
        adjacencyFileLoaded=false;
        dotFileLoaded=false;
        graphMLFileLoaded=false;
        fileLoaded=true;
        networkModified=false;
        statusMessage( QString(tr("Two-mode affiliation network, named %1, loaded with %2 Nodes and %3 total Links.")).arg( networkName ).arg( aNodes ).arg(totalLinks ) );
        break;

    default: // just for sanity
        pajekFileLoaded=false;
        adjacencyFileLoaded=false;
        graphMLFileLoaded=false;
        fileLoaded=false;
        QMessageBox::critical(this, "Error","Unrecognized format. \nPlease specify"
                              " which is the file-format using Import Menu.","OK",0);
        break;
    }
    graphChanged();
    fileSave->setIcon(QIcon(":/images/saved.png"));
    fileSave->setEnabled(false);
}








/**
    Calls Graph::createVertex method to add a new RANDOM node into the activeGraph.
    Called when "Create Node" button is clicked on the Main Window.
*/
void MainWindow::addNode() {
    qDebug("MW: addNode(). Calling activeGraph::createVertex() for -1 - max width and height %i, %i", graphicsWidget->width()-10,  graphicsWidget->height()-10);
    activeGraph.createVertex(-1, graphicsWidget->width()-10,  graphicsWidget->height()-10);  // minus a  screen edge offset...
    statusMessage( tr("New node (numbered %1) added.").arg(activeGraph.lastVertexNumber())  );
}


/**
    Calls Graph::createVertex method to add a new node into the activeGraph.
    Called on double clicking
*/
void MainWindow::addNodeWithMouse(int num, QPointF p) {
    qDebug("MW: addNodeWithMouse(). Calling activeGraph::createVertex() for a vertice named %i", num);
    activeGraph.createVertex(num, p);
    statusMessage( tr("New node (numbered %1) added.").arg(activeGraph.lastVertexNumber())  );
}





/**
    Exports the network to a PNG image
    Mediocre Quality but smaller file
*/

bool MainWindow::slotExportPNG(){
    qDebug("slotExportPNG");
    if (!fileLoaded && !networkModified )  {
        QMessageBox::critical(this, "Error",tr("The canvas is empty!\nLoad a network file or create a new network first."), "OK",0);
        statusMessage( tr("Cannot export PNG.") );
        return false;
    }
    QString fn = QFileDialog::getSaveFileName(this,tr("Save"), 0, tr("Image Files (*.png)"));
    if (fn.isEmpty())  {
        statusMessage( tr("Saving aborted") );
        return false;
    }
    tempFileNameNoPath=fn.split ("/");
    qDebug("slotExportPNG: grabing canvas");
    QPixmap picture;
    picture=QPixmap::grabWidget(graphicsWidget, graphicsWidget->rect());
    qDebug("slotExportPNG: adding logo");
    QPainter p;
    p.begin(&picture);
    p.setFont(QFont ("Helvetica", 10, QFont::Normal, false));
    p.drawText(5,10,"SocNetV: "+tempFileNameNoPath.last());
    p.end();
    qDebug("slotExportPNG: checking filename");
    if (fn.contains("png", Qt::CaseInsensitive) ) {
        picture.toImage().save(fn, "PNG");
        QMessageBox::information(this, "Export to PNG...",tr("Image Saved as: ")+tempFileNameNoPath.last(), "OK",0);
    }
    else {
        picture.toImage().save(fn+".png", "PNG");
        QMessageBox::information(this, "Export to PNG...",tr("Image Saved as: ")+tempFileNameNoPath.last()+".png" , "OK",0);
    }

    statusMessage( tr("Exporting completed") );

    return true;
}



/**
    Exports the network to a BMP image
    Better Quality but larger file
*/
bool MainWindow::slotExportBMP(){
    qDebug(	"slotExportBMP()");
    if (!fileLoaded && !networkModified )  {
        QMessageBox::critical(this, "Error",tr("Nothing to export! \nLoad a network file or create a new network first."), "OK",0);
        statusMessage( tr("Cannot export BMP.") );
        return false;
    }
    QString format="bmp";
    QString fn = QFileDialog::getSaveFileName(this,tr("Save Image as"), 0,tr("Image Files (*.bmp)"));
    if (fn.isEmpty())  {
        statusMessage( tr("Saving aborted") );
        return false;
    }
    tempFileNameNoPath=fn.split ("/");

    QPixmap picture;
    qDebug("slotExportBMP: grabing canvas");
    picture=QPixmap::grabWidget(graphicsWidget, graphicsWidget->viewport()->rect());
    QPainter p;
    qDebug("slotExportBMP: adding logo");
    p.begin(&picture);
    p.setFont(QFont ("Helvetica", 10, QFont::Normal, false));
    p.drawText(5,10,"SocNetV: "+tempFileNameNoPath.last());
    p.end();
    qDebug("slotExportBMP: checking file");
    if (fn.contains(format, Qt::CaseInsensitive) ) {
        picture.toImage().save(fn, format.toLatin1());
        QMessageBox::information(this, tr("Export to BMP..."),tr("Image Saved as: ")+tempFileNameNoPath.last(), "OK",0);
    }
    else {
        picture.toImage().save(fn+"."+format, format.toLatin1());
        QMessageBox::information(this, tr("Export to BMP..."),tr("Image Saved as: ")+tempFileNameNoPath.last()+"."+format , "OK",0);
    }
    qDebug()<< "Exporting BMP to "<< fn;

    statusMessage( tr("Exporting completed") );
    qDebug("Export finished!");
    return true;
}






/**
    Exports the network to a PDF Document
    Best Quality
*/
bool MainWindow::slotExportPDF(){
    qDebug(	"slotExportPDF()");
    if (!fileLoaded && !networkModified )  {
        QMessageBox::critical(this, "Error",tr("The canvas is empty!\nLoad a network file or create a new network first."), "OK",0);
        statusMessage( tr("Cannot export PDF.")  );
        return false;
    }

    QString m_fileName = QFileDialog::getSaveFileName(this, tr("Export to PDF"), 0, tr("Portable Document Format files (*.pdf)"));
    if (m_fileName.isEmpty())  {
        statusMessage( tr("Saving aborted"));
        return false;
    }
    else {
        if (QFileInfo(m_fileName).suffix().isEmpty())
            m_fileName.append(".pdf");

        QPrinter printer(QPrinter::HighResolution);
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName(m_fileName);
        QPainter painter(&printer);
        graphicsWidget->render(&painter);
    }
    qDebug()<< "Exporting PDF to "<< m_fileName;
    tempFileNameNoPath=m_fileName.split ("/");
    QMessageBox::information(this, tr("Export to PDF..."),tr("File saved as: ")+tempFileNameNoPath.last() , "OK",0);
    statusMessage(  tr("Exporting completed") );
    return true;
}




/**
    Exports the network to a Pajek-formatted file
    Calls the relevant Graph method.
*/
void MainWindow::slotExportPajek()
{
    qDebug ("MW: slotExportPajek");

    if (!fileLoaded && !networkModified )  {
        QMessageBox::critical(this, "Error",tr("Nothing to export! \nLoad a network file or create a new network first."), "OK",0);
        statusMessage( tr("Cannot export to Pajek.")  );
        return;
    }

    statusMessage( tr("Exporting active network under new filename..."));
    QString fn =  QFileDialog::getSaveFileName(this,
                                               tr("Export Network to File Named..."),
                                               0, tr("Pajek (*.paj *.net *.pajek);;All (*)") );
    if (!fn.isEmpty())  {
        if  ( QFileInfo(fn).suffix().isEmpty() ){
            QMessageBox::information(this, "Missing Extension ",tr("File extension was missing! \nI am appending a standard .paj to the given filename."), "OK",0);
            fn.append(".paj");
        }
        fileName=fn;
        fileNameNoPath=fileName.split ("/");
    }
    else  {
        statusMessage( tr("Saving aborted"));
        return;
    }

    int maxWidth=scene->width();
    int maxHeight=scene->height();

    if ( activeGraph.saveGraph(fileName, 1, networkName, maxWidth,maxHeight ) )
        networkSaved(1);
    else
        networkSaved(0);
}



/**	Exports the network to a adjacency matrix-formatted file
    Calls the relevant Graph method.
*/
void MainWindow::slotExportSM(){
    qDebug("MW: slotExportSM()");
    if (!fileLoaded && !networkModified )  {
        QMessageBox::critical(this, "Error",tr("Nothing to export!\nLoad a network file or create a new network first."), "OK",0);
        statusMessage( tr("Cannot export to Adjacency Matrix.")  );
        return;
    }
    statusMessage( tr("Exporting active network under new filename..."));
    QString fn =  QFileDialog::getSaveFileName(this,
                                               tr("Export Network to File Named..."),
                                               0, tr("Adjacency (*.adj *.sm *.txt *.csv *.net);;All (*)") );
    if (!fn.isEmpty())  {
        if  ( QFileInfo(fn).suffix().isEmpty() ){
            QMessageBox::information(this, "Missing Extension ",tr("File extension was missing! \nI am appending a standard .adj to the given filename."), "OK",0);
            fn.append(".adj");
        }
        fileName=fn;
        fileNameNoPath=fileName.split ("/");
    }
    else  {
        statusMessage( tr("Saving aborted"));
        return;
    }

    QMessageBox::information(this, "Warning",tr("Note that exporting to an adjacency matrix does not save floating-point weight values; adjacency matrices consist of integers, only. \n If your network had any floating point weights in some edges, these are being truncated to the nearest integer or 1."), "OK",0);
    int maxWidth=scene->width();
    int maxHeight=scene->height();

    if ( activeGraph.saveGraph(fileName, 2, networkName,maxWidth,maxHeight ) )
        networkSaved(1);
    else
        networkSaved(0);
}






/**
    Exports the network to a DL-formatted file
    TODO slotExportDL
*/
bool MainWindow::slotExportDL(){
    if (!fileLoaded && !networkModified )  {
        QMessageBox::critical(this, "Error",tr("Nothing to export!\nLoad a network file or create a new network first."), "OK",0);
        statusMessage( tr("Cannot export to DL.")  );
        return false;
    }

    if (fileName.isEmpty()) {
        statusMessage( tr("Saving network under new filename..."));
        QString fn = QFileDialog::getSaveFileName(this, 0, 0);
        if (!fn.isEmpty())  {
            fileName=fn;
        }
        else  {
            statusMessage( tr("Saving aborted"));
            return false;
        }
    }

    return true;

}


/**
    Exports the network to a GW-formatted file
    TODO slotExportGW
*/ 
bool MainWindow::slotExportGW(){
    if (!fileLoaded && !networkModified )  {
        QMessageBox::critical(this, "Error",tr("Nothing to export!\nLoad a network file or create a new network first."), "OK",0);
        statusMessage( tr("Cannot export to GW.")  );
        return false;
    }

    if (fileName.isEmpty()) {
        statusMessage( tr("Saving network under new filename..."));
        QString fn = QFileDialog::getSaveFileName(this, 0, 0);
        if (!fn.isEmpty())  {
            fileName=fn;
        }
        else  {
            statusMessage( tr("Saving aborted"));
            return false;
        }
    }

    return true;
}




/**
    Exports the network to a list-formatted file
    TODO slotExportList
*/
bool MainWindow::slotExportList(){
    if (fileName.isEmpty()) {
        statusMessage( tr("Saving network under new filename..."));
        QString fn = QFileDialog::getSaveFileName(this, 0, 0);
        if (!fn.isEmpty())  {
            fileName=fn;
        }
        else  {
            statusMessage( tr("Saving aborted"));
            return false;
        }
    }

    return true;
}





/**
    Adds a little universal randomness :)
*/
void MainWindow::makeThingsLookRandom()   {
    time_t now;				/* define 'now'. time_t is probably a typedef	*/
    now = time((time_t *)NULL);		/* Get the system time and put it
                     * into 'now' as 'calender time' the number of seconds since  1/1/1970   	*/

    srand( (unsigned int ) now);
}




/**
    Displays the file of the loaded network.
    Network _must_ be unchanged since last save/load.
    Otherwise it will ask the user to first save the network, then view its file.
*/
void MainWindow::slotViewNetworkFile(){
    qDebug() << "slotViewNetworkFile() : " << fileName.toLatin1();
    if ( fileLoaded && !networkModified ) { //file network unmodified
        QFile f( fileName );
        if ( !f.open( QIODevice::ReadOnly ) ) {
            qDebug ("Error in open!");
            return;
        }
        TextEditor *ed = new TextEditor(fileName);//OPEN A TEXT EDITOR WINDOW
        ed->setWindowTitle(tr("Viewing network file - ") + fileNameNoPath.last() );
        ed->show();
        statusMessage(  tr("Loaded network text file " )+ fileNameNoPath.last()  );
    }
    else if (fileName.isEmpty() && networkModified)     {  //New network + something
        QMessageBox::information (this, "Viewing network file",
                                  tr("This network has not been saved yet. \nI will open a dialog for you to save it now. \nPlease choose a filename..."), "OK",0);
        slotFileSaveAs();
    }
    else if (fileLoaded && networkModified ) {   //file network + modified
        QMessageBox::information (this, "Viewing network file",
                                  //FIXME maybe better to save automagically rather than asking?
                                  tr("The network has been modified. \nI will save it to the original file for you now."), "OK",0);
        networkModified = false;
        slotFileSave();
        slotViewNetworkFile();
    }
    else	{
        QMessageBox::critical(this, "Error",
                              tr("Empty network! \nLoad a network file first or create and save a new one..."), "OK",0);
        statusMessage(  tr("Nothing here. Not my fault, though!") );
    }
}




/**
    Opens the embedded text editor
*/
void MainWindow::slotOpenTextEditor(){
    qDebug() << "slotOpenTextEditor() : ";

    TextEditor *ed = new TextEditor("");
    ed->setWindowTitle(tr("New Network File"));
    ed->show();
    statusMessage(  tr("Enter your network data here" ) );
}





/**
    Displays the adjacency matrix of the network.
    It uses a different method for writing the matrix to a file.
    While slotExportSM uses << operator of Matrix class (via adjacencyMatrix of Graph class),
    this is using directly the writeAdjacencyMatrix method of Graph class
*/
void MainWindow::slotViewAdjacencyMatrix(){
    if ( !fileLoaded && !networkModified) {
        QMessageBox::critical (this, "Error",
                               tr("Empty network! \nLoad a network file or create something by double-clicking on the canvas!"), "OK",0);

        statusMessage(  tr("Nothing to show!") );
        return;
    }
    int aNodes=activeNodes();
    statusBar() ->  showMessage ( QString (tr ("creating adjacency adjacency matrix of %1 nodes")).arg(aNodes) );
    qDebug ("MW: calling Graph::writeAdjacencyMatrix with %i nodes", aNodes);
    char fn[]= "adjacency-matrix.dat";

    activeGraph.writeAdjacencyMatrix(fn, networkName.toLocal8Bit()) ;

    //Open a text editor window for the new file created by graph class
    QString qfn=QString::fromLocal8Bit("adjacency-matrix.dat");
    TextEditor *ed = new TextEditor(fn);
    tempFileNameNoPath=qfn.split( "/");
    ed->setWindowTitle(tr("View Adjacency Matrix - ") + tempFileNameNoPath.last());
    ed->show();

}



/*
 * Calls the m_datasetSelectionDialog to display the datasetselection dialog
 */
void MainWindow::slotShowDataSetSelectDialog(){
    qDebug()<< "slotShowDataSetSelectDialog()";
    m_datasetSelectDialog.exec();
}


/*
 * Recreates some of the most famous and widely used data sets
 * in network analysis studies
 */
void MainWindow::slotRecreateDataSet (QString m_fileName) {
    int m_fileFormat=0;
    qDebug()<< "slotRecreateDataSet() fileName: " << m_fileName;

    initNet();
    activeGraph.writeDataSetToFile(m_fileName);

    if (m_fileName.endsWith(".graphml")) {
        m_fileFormat=1;
    }
    else if (m_fileName.endsWith(".pajek")) {
        m_fileFormat=2;
    }
    else if (m_fileName.endsWith(".sm") || m_fileName.endsWith(".adj")) {
        m_fileFormat=3;
    }
    else if (m_fileName.endsWith(".dot")) {
        m_fileFormat=4;
    }
    else if (m_fileName.endsWith(".gml")) {
        m_fileFormat=5;
    }
    else if (m_fileName.endsWith(".dl")) {
        m_fileFormat=6;
    }
    else if (m_fileName.endsWith(".list")) {
        m_fileFormat=7;
    }
    else if (m_fileName.endsWith(".lst")) {
        m_fileFormat=8;
    }
    else if (m_fileName.endsWith(".2sm")) {
        m_fileFormat=9;
    }


    if ( loadNetworkFile(m_fileName, m_fileFormat) ) {
        fileNameNoPath=m_fileName.split ("/" );
        fileName=m_fileName;
        previous_fileName=fileName;
        setWindowTitle("SocNetV "+ VERSION +" - "+fileNameNoPath.last());
        QString message=tr("Network saved as ")+fileNameNoPath.last();
        statusMessage( message );
    }
    else {
        statusMessage( "Could not read new network data file. Aborting.");
    }
}

/**
    Calls activeGraph.createRandomNetErdos () to create a symmetric network
    Link existance is controlled by a user specified possibility.
*/
void MainWindow::slotCreateRandomNetErdos(){
    bool ok;
    statusMessage( "You have selected to create a random symmetric network. ");
    int newNodes=( QInputDialog::getInt(this, "Create random network", tr("This will create a new random symmetric network of G(n,p) model, \nwhere n is the nodes and p is the edge probability. \nPlease enter the number n of nodes you want:"),20, 1, maxNodes, 1, &ok ) ) ;
    if (!ok) {
        statusMessage( "You did not enter an integer. Aborting.");
        return;
    }
    double probability= QInputDialog::getDouble(this,"Create random network", "Enter an edge probability % (0-100):", 4, 0, 100, 1, &ok );
    if (!ok) {
        statusMessage( "You did not enter an integer. Aborting.");
        return;
    }
    statusMessage( "Erasing any existing network. ");

    initNet();
    makeThingsLookRandom();
    statusMessage( tr("Creating random network. Please wait... ")  );

    qDebug("MW Erdos network:  Create random network of %i nodes and %f edge probability.",newNodes, probability);

    if (showProgressBarAct->isChecked() && newNodes > 300){
        progressDialog= new QProgressDialog("Creating random network. Please wait (or disable me from Options > View > ProgressBar, next time ;)).", "Cancel", 0, newNodes+newNodes, this);
        progressDialog -> setWindowModality(Qt::WindowModal);
        connect( &activeGraph, SIGNAL( updateProgressDialog(int) ), progressDialog, SLOT(setValue(int) ) ) ;
        progressDialog->setMinimumDuration(0);
    }

    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

    activeGraph.createRandomNetErdos (newNodes, probability);
    QApplication::restoreOverrideCursor();

    if (showProgressBarAct->isChecked() && newNodes > 300)
        progressDialog->deleteLater();

    fileLoaded=false;

    graphChanged();

    setWindowTitle("Untitled");
    double threshold = log(newNodes)/newNodes;
    //float avGraphDistance=activeGraph.averageGraphDistance();
    //float clucof=activeGraph.clusteringCoefficient();
    if ( (probability/100 ) > threshold )
        QMessageBox::information(this, "New Random Network",
                                 tr("Random network created. \n")+
                                 tr("\nNodes: ")+ QString::number(activeNodes())+
                                 tr("\nEdges: ")+  QString::number( activeLinks()/2.0)+
                                 //tr("\nAverage path length: ") + QString::number(avGraphDistance)+
                                 //tr("\nClustering coefficient: ")+QString::number(clucof)+
                                 tr("\n\nOn the average, edges should be ") + QString::number(probability * newNodes*(newNodes-1)/100) +
                                 tr("\nThis graph is almost surely connected because: \nprobability > ln(n)/n, that is: \n") + QString::number(probability/100)+
                                 tr(" bigger than ")+ QString::number(threshold) , "OK",0);

    else
        QMessageBox::information(this, "New Random Network",
                                 tr("Random network created. \n")+
                                 tr("\nNodes: ")+ QString::number(activeNodes())+
                                 tr("\nEdges: ")+  QString::number( activeLinks()/2.0)+
                                 //tr("\nAverage path length: ") + QString::number(avGraphDistance)+
                                 //tr("\nClustering coefficient: ")+QString::number(clucof)+
                                 tr("\n\nOn the average, edges should be ") +QString::number(probability * newNodes*(newNodes-1)/100) +
                                 tr("\nThis graph is almost surely not connected because: \nprobability < ln(n)/n, that is: \n") +
                                 QString::number(probability/100)+ " smaller than "+ QString::number(threshold) , "OK",0);

    statusMessage( "Random network created. ");

}













/**
    Creates a pseudo-random network where every node has the same degree
*/
void MainWindow::slotCreateSameDegreeRandomNetwork(){
    bool ok;
    statusMessage( "You have selected to create a pseudo-random network where each node has the same degree. ");
    int newNodes=( QInputDialog::getInt(this, "Create same degree network", tr("This will create a same degree network. \nPlease enter the number of nodes you want:"),20, 1, maxNodes, 1, &ok ) ) ;
    if (!ok) {
        statusMessage( "You did not enter an integer. Aborting.");
        return;
    }
    int degree = QInputDialog::getInt(this,"Create same degree network...", "Now, select an even number d. \nThis will be the number of links of each node:", 2, 2, newNodes-1, 2, &ok);
    if ( (degree% 2)==1 ) {
        QMessageBox::critical(this, "Error",tr(" Sorry. I cannot create such a network. Links must be even number"), "OK",0);
        return;
    }
    statusMessage( "Erasing any existing network. ");
    initNet();
    makeThingsLookRandom();
    statusMessage( "Creating a pseudo-random network where each node has the same degree... ");

    if (showProgressBarAct->isChecked() && newNodes > 300){
        progressDialog= new QProgressDialog("Creating random network. Please wait (or disable me from Options > View > ProgressBar, next time ;)).", "Cancel", 0, (int) (newNodes+newNodes), this);
        progressDialog -> setWindowModality(Qt::WindowModal);
        connect( &activeGraph, SIGNAL( updateProgressDialog(int) ), progressDialog, SLOT(setValue(int) ) ) ;
        progressDialog->setMinimumDuration(0);
    }

    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

    activeGraph.createSameDegreeRandomNetwork (newNodes, degree);

    QApplication::restoreOverrideCursor();

    if (showProgressBarAct->isChecked() && newNodes > 300)
        progressDialog->deleteLater();

    fileLoaded=false;

    graphChanged();
    setWindowTitle("Untitled");
    statusMessage( "Uniform random network created: "+QString::number(activeNodes())+" Nodes, "+QString::number( activeLinks())+" Links");

}


void MainWindow::slotCreateGaussianRandomNetwork(){
    graphChanged();

}



void MainWindow::slotCreateSmallWorldRandomNetwork(){
    bool ok=false;
    statusMessage( "You have selected to create a small world network.");
    int newNodes=( QInputDialog::getInt(this, "Create small world",
                                        tr("This will create a small world network, \n")+
                                        tr("that is an undirected graph with N nodes and N*d/2 edges,\n")+
                                        tr("where d is the mean edge degree.\n")+
                                        tr("Please enter the number N of nodes you want:"),
                                        20, 1, maxNodes, 1, &ok ) ) ;
    if (!ok) {
        statusMessage( "You did not enter an integer. Aborting.");
        return;
    }
    int degree = QInputDialog::getInt(this,"Create small world...",
                                      tr("Now, enter an even number d. \n")+
                                      tr("This is the mean edge degree each new node will have:"),
                                      2, 2, newNodes-1, 2, &ok);
    if ( (degree% 2)==1 ) {
        QMessageBox::critical(this, "Error",tr(" Sorry. I cannot create such a network. Links must be even number"), "OK",0);
        return;
    }
    double beta = QInputDialog::getDouble(this,"Create small world...",
                                          tr("Now, enter a parameter beta. \n")+
                                          tr("This is the edge rewiring probability:"),
                                          0.6, 0.00, 1.00, 2, &ok);

    statusMessage( tr("Erasing any existing network. "));
    initNet();
    makeThingsLookRandom();
    statusMessage( tr("Creating small world. Please wait..."));
    double x0=scene->width()/2.0;
    double y0=scene->height()/2.0;
    double radius=(graphicsWidget->height()/2.0)-50;          //pixels

    if (showProgressBarAct->isChecked() && newNodes > 300){
        progressDialog= new QProgressDialog(
                    tr("Creating random network. Please wait \n (or disable me from Options > View > ProgressBar, next time )."),
                    "Cancel", 0, (int) (newNodes+newNodes), this);
        progressDialog -> setWindowModality(Qt::WindowModal);
        connect( &activeGraph, SIGNAL( updateProgressDialog(int) ), progressDialog, SLOT(setValue(int) ) ) ;
        progressDialog->setMinimumDuration(0);
    }
    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
    activeGraph.createRandomNetSmallWorld(newNodes, degree, beta, x0, y0, radius);
    QApplication::restoreOverrideCursor();

    if (showProgressBarAct->isChecked() && newNodes > 300 )
        progressDialog->deleteLater();

    fileLoaded=false;

    graphChanged();
    setWindowTitle("Untitled");
    statusMessage( tr("Small world random network created: ")+QString::number(activeNodes())+" nodes, "+QString::number( activeLinks())+" links");
    //float avGraphDistance=activeGraph.averageGraphDistance();
    //float clucof=activeGraph.clusteringCoefficient();
    QMessageBox::information(this, "New Small World",
                             tr("Small world network created.\n")+
                             tr("\nNodes: ")+ QString::number(activeNodes())+
                             tr("\nEdges: ")+  QString::number( activeLinks()/2.0)
                             //+  tr("\nAverage path length: ") + QString::number(avGraphDistance)
                             //+ tr("\nClustering coefficient: ")+QString::number(clucof)
                             , "OK",0);

}





/**
    Creates a lattice network, i.e. a connected network where every node
    has the same degree and is linked with its neighborhood.
*/
void MainWindow::slotCreateRandomNetRingLattice(){
    bool ok;
    statusMessage( "You have selected to create a ring lattice network. ");
    int newNodes=( QInputDialog::getInt(this, "Create ring lattice", tr("This will create a ring lattice network, where each node has degree d:\n d/2 edges to the right and d/2 to the left.\n Please enter the number of nodes you want:"),20, 1, maxNodes, 1, &ok ) ) ;
    if (!ok) {
        statusMessage( "You did not enter an integer. Aborting.");
        return;
    }
    int degree = QInputDialog::getInt(this,"Create ring lattice...", "Now, enter an even number d. \nThis is the total number of links each new node will have:", 2, 2, newNodes-1, 2, &ok);
    if ( (degree% 2)==1 ) {
        QMessageBox::critical(this, "Error",tr(" Sorry. I cannot create such a network. Links must be even number"), "OK",0);
        return;
    }

    statusMessage( "Erasing any existing network. ");
    initNet();
    makeThingsLookRandom();
    statusMessage( "Creating ring lattice network. Please wait...");
    double x0=scene->width()/2.0;
    double y0=scene->height()/2.0;
    double radius=(graphicsWidget->height()/2.0)-50;          //pixels

    if (showProgressBarAct->isChecked() && newNodes > 300){
        progressDialog= new QProgressDialog("Creating random network. Please wait (or disable me from Options > View > ProgressBar, next time ;)).", "Cancel", 0, (int) (newNodes+newNodes), this);
        progressDialog -> setWindowModality(Qt::WindowModal);
        connect( &activeGraph, SIGNAL( updateProgressDialog(int) ), progressDialog, SLOT(setValue(int) ) ) ;
        progressDialog->setMinimumDuration(0);
    }

    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

    activeGraph.createRandomNetRingLattice(newNodes, degree, x0, y0, radius );

    QApplication::restoreOverrideCursor();

    if (showProgressBarAct->isChecked() && newNodes > 300)
        progressDialog->deleteLater();

    fileLoaded=false;

    //	graphChanged();

    statusMessage( "Ring lattice random network created: "+QString::number(activeNodes())+" nodes, "+QString::number( activeLinks())+" links");

    setWindowTitle("Untitled");
    //float avGraphDistance=activeGraph.averageGraphDistance();
    //float clucof=activeGraph.clusteringCoefficient();
    QMessageBox::information(this, "Ring Lattice",
                             tr("Ring lattice network created.\n")+
                             tr("\nNodes: ")+ QString::number(activeNodes())+
                             tr("\nEdges: ")+  QString::number( activeLinks()/2.0)
                             // + tr("\nAverage path length: ") + QString::number(avGraphDistance)
                             //+ tr("\nClustering coefficient: ")+QString::number(clucof)
                             , "OK",0);
}







/**
*	Shows a dialog from where the user   
*	creates a new network by crawling a given website 
*/ 
void MainWindow::slotShowWebCrawlerDialog() {
    qDebug () << "MW: slotShowWebCrawlerDialog() - sending canvasWidth and Height";
    activeGraph.setCanvasDimensions(graphicsWidget->width(), graphicsWidget->height());

    m_WebCrawlerDialog.exec() ;
}






/**
*	Called from m_WebCrawlerDialog
*   Clears the loaded network (saving if needed)    
*	then passes parameters to webCrawl of ActiveGraph class.  
*/ 
void MainWindow::slotWebCrawl(QString  seed, int maxNodes, int maxRecursion,  bool goOut ) {
    this->slotFileClose();
    activeGraph.webCrawl( seed, maxNodes, maxRecursion,  goOut ) ;
}



/**
     Calls GW: findNode() to find a node by its number or label. The node is then marked.
*/
void MainWindow::slotFindNode(){
    qDebug ("MW: slotFindNode()");
    if (!fileLoaded && !networkModified  ) {
        QMessageBox::critical( this, tr("Find Node"),
                               tr("No nodes present! \nLoad a network file first or create some nodes..."),
                               tr("OK"),0 );
        statusMessage(  QString(tr("Nothing to find!"))  );
        return;
    }

    if ( markedNodeExists ) {				// if a node has been already marked
        graphicsWidget->setMarkedNode(""); 	// call setMarkedNode to just unmark it.
        markedNodeExists=false;
        statusMessage( tr("Node unmarked.") );
        return;								// and return to MW
    }

    bool ok=false;
    QString nodeText = QInputDialog::getText(this, tr("Find Node"),
                                             tr("Enter node label or node number:"),
                                             QLineEdit::Normal,QString::null, &ok );
    if (!ok) {
        statusMessage( tr("Find node operation cancelled.") );
        return;
    }

    else {
        if	( graphicsWidget->setMarkedNode(nodeText) ) {
            markedNodeExists=true;
            statusMessage( tr("Node found and marked. Press Ctrl+F again to unmark...") );
        }
        else {
            QMessageBox::information(this, tr("Find Node"),
                                     tr("Sorry. There is no such node in this network. \n Try again."), "OK",0);
        }
    }
}



/*
 * Called by Graph to update what the selected node is.
 */
void MainWindow::selectedNode (const int vertex){
    clickedJimNumber=vertex;
}



/**
     Popups a context menu with some options when the user right-clicks on a node
*/
void MainWindow::openNodeContextMenu() {
    clickedJimNumber=clickedJim->nodeNumber();
    qDebug("MW: openNodeContextMenu() for node %i at %i, %i",clickedJimNumber, QCursor::pos().x(), QCursor::pos().y());

    QMenu *nodeContextMenu = new QMenu(QString::number(clickedJimNumber), this);
    Q_CHECK_PTR( nodeContextMenu );  //displays "out of memory" if needed

    nodeContextMenu -> addAction(addLinkAct);
    nodeContextMenu -> addAction(removeNodeAct );
    QMenu *options=new QMenu(tr("Options"), this);
    nodeContextMenu -> addMenu( options );
    options -> addAction( changeNodeLabelAct );
    options -> addAction( changeNodeSizeAct );
    options -> addAction( changeNodeValueAct );
    options -> addAction( changeNodeColorAct );
    options -> addAction( changeNodeBoxAct );
    options -> addAction( changeNodeCircleAct );
    options -> addAction( changeNodeDiamondAct );
    options -> addAction( changeNodeEllipseAct);
    options -> addAction( changeNodeTriangleAct );
    //QCursor::pos() is good only for menus not related with node coordinates
    nodeContextMenu -> exec(QCursor::pos() );
    delete  nodeContextMenu;
    clickedJimNumber=-1;    //undo node selection
}



/**
     Popups a context menu with some options when the user right-clicks on a link
*/
void MainWindow::openLinkContextMenu() {
    int source=clickedLink->sourceNodeNumber();
    int target=clickedLink->targetNodeNumber();
    qDebug("MW: openLinkContextMenu() for edge %i-%i at %i, %i",source, target, QCursor::pos().x(), QCursor::pos().y());
    QString edgeName=QString::number(source)+QString("-")+QString::number(target);
    //make the menu
    QMenu *linkContextMenu = new QMenu(edgeName, this);
    linkContextMenu -> addAction( removeLinkAct );
    linkContextMenu -> addAction( changeLinkWeightAct );
    linkContextMenu -> addAction( changeLinkColorAct );
    linkContextMenu -> exec(QCursor::pos() );
    delete  linkContextMenu;
}

/**
     Popups a context menu with some options when the user right-clicks on the scene
*/
void MainWindow::openContextMenu( const QPointF &mPos) {
    cursorPosGW=mPos;
    QMenu *contextMenu = new QMenu("Link Menu",this);
    Q_CHECK_PTR( contextMenu );  //displays "out of memory" if needed
    contextMenu -> addAction( addNodeAct );
    QMenu *options=new QMenu("Options", this);
    contextMenu -> addMenu(options );
    options -> addAction(changeBackColorAct  );
    options -> addAction(changeAllNodesSizeAct );
    options -> addAction(changeAllNodesShapeAct  );
    options -> addAction(changeAllNodesColorAct );
    options -> addAction(changeAllLinksColorAct  );
    options -> addAction (displayNodeNumbersAct);
    options -> addAction (displayNodeLabelsAct);
    //QCursor::pos() is good only for menus not related with node coordinates
    contextMenu -> exec(QCursor::pos() );
    delete  contextMenu;
    cursorPosGW=QPoint(-1,-1);
}







/**
*	A slot activated when something has been changed in the graph.
    Makes the fileSave icon active and refreshes any LCD values.
    Also called from graphicsWidget.
*/
void MainWindow::graphChanged(){
    qDebug("MW: graphChanged");
    networkModified=true;
    fileSave->setIcon(QIcon(":/images/save.png"));
    fileSave->setEnabled(true);

    nodesLCD->display(activeGraph.vertices());
    edgesLCD->display(activeGraph.totalEdges());
    densityLCD->display( activeGraph.density() );
}



/**
*	When the user clicks on a node, displays some information about it on the status bar.
*/
void MainWindow::nodeInfoStatusBar ( Node *jim) {
    qDebug ("MW: NodeInfoStatusBar()");
    linkClicked=false;
    nodeClicked=true;
    clickedJim=jim;
    clickedJimNumber=clickedJim->nodeNumber();
    int inLinks=activeGraph.inDegree(clickedJimNumber);
    int outLinks=activeGraph.outDegree(clickedJimNumber);
    selectedNodeLCD->display (clickedJimNumber);
    inLinksLCD->display (inLinks);
    outLinksLCD->display (outLinks);
    clucofLCD->display(activeGraph.clusteringCoefficient(clickedJimNumber));

    statusMessage(  QString(tr("(%1, %2);  Node %3, with label %4, "
                               "has %5 in-Links and %6 out-Links.")).arg( ceil( clickedJim->x() ) )
                    .arg( ceil( clickedJim->y() )).arg( clickedJimNumber ).arg( clickedJim->labelText() )
                    .arg(inLinks).arg(outLinks) );
    clickedJimNumber=-1;
}



/**
*	When the user clicks on a link, displays some information about it on the status bar.
*/
void MainWindow::linkInfoStatusBar (Edge* link) {
    clickedLink=link;
    linkClicked=true;
    nodeClicked=false;
    statusMessage(  QString(tr("Link between node %1 and node %2, weight %3 and color %4.")).arg( link->sourceNodeNumber() ).arg(link->targetNodeNumber()).arg(link->weight()).arg(link->color() ) );
}







/**
* 	Deletes a node and the attached objects (links, etc).
*	It deletes clickedJim (signal from GraphicsView or set by another function) 
*	or else asks for a nodeNumber to remove. The nodeNumber is doomedJim.
    Called from nodeContextMenu
*/
void MainWindow::slotRemoveNode() {
    qDebug("MW: slotRemoveNode()");
    if (!activeGraph.vertices())  {
        QMessageBox::critical(this, "Error",tr("Nothing to do! \nLoad a network file or add some nodes first."), "OK",0);
        statusMessage( tr("Nothing to remove.")  );
        return;
    }
    int doomedJim=-1, min=-1, max=-1;
    bool ok=false;

    min = activeGraph.firstVertexNumber();
    max = activeGraph.lastVertexNumber();
    qDebug("MW: min is %i and max is %i", min, max);
    if (min==-1 || max==-1 ) {
        qDebug("ERROR in finding min max nodeNumbers. Abort");
        return;
    }
    else if (clickedJimNumber >= 0 && clickedJimNumber<= max ) {
        doomedJim=clickedJimNumber ;
    }
    else if (clickedJimNumber == -1 ) {
        doomedJim =  QInputDialog::getInt(this,"Remove node",tr("Choose a node to remove between ("
                                                                + QString::number(min).toLatin1()+"..."+QString::number(max).toLatin1()+"):"),min, 1, max, 1, &ok);
        if (!ok) {
            statusMessage( "Remove node operation cancelled." );
            return;
        }
    }
    qDebug ("MW: removing vertice with number %i from Graph", doomedJim);
    activeGraph.removeVertex(doomedJim);
    clickedJimNumber=-1;
    graphChanged();
    qDebug("MW: removeNode() completed. Node %i removed completely.",doomedJim);
    statusMessage( tr("Node removed completely. Ready. ") );
}





/**
*	Adds a new link between two nodes specified by the user.
    Called when user clicks on the MW button "Add Link".
*/
void MainWindow::slotAddLink(){
    qDebug ("MW: slotAddLink()");
    if (!fileLoaded && !networkModified )  {
        QMessageBox::critical(this, "Error",tr("Nothing to link to! \nCreate some nodes first."), "OK",0);
        statusMessage( tr("There are no nodes yet...")  );
        return;
    }

    int sourceNode=-1, targetNode=-1, sourceIndex=-1, targetIndex=-1;
    float weight=1; 	//weight of this new edge should be one...
    bool ok=false;
    int min=activeGraph.firstVertexNumber();
    int max=activeGraph.lastVertexNumber();

    if (min==max) return;		//if there is only one node -> no link

    if (clickedJimNumber == -1) {
        sourceNode=QInputDialog::getInt(this, "Create new link, Step 1",tr("This will draw a new link between two nodes. \nEnter source node ("+QString::number(min).toLatin1()+"..."+QString::number(max).toLatin1()+"):"), min, 1, max , 1, &ok ) ;
        if (!ok) {
            statusMessage( "Add link operation cancelled." );
            return;
        }
    }
    else sourceNode=clickedJimNumber;

    if ( (sourceIndex =activeGraph.hasVertex(sourceNode)) ==-1 ) {
        statusMessage( tr("Aborting. ")  );
        QMessageBox::critical(this,"Error","No such node.", "OK",0);
        qDebug ("MW: slotAddLink: Cant find sourceNode %i.", sourceNode);
        return;
    }

    targetNode=QInputDialog::getInt(this, "Create new link, Step 2", tr("Source node accepted. \nNow enter target node ("+QString::number(min).toLatin1()+"..."+QString::number(max).toLatin1()+"):"),min, 1, max , 1, &ok)     ;
    if (!ok) {
        statusMessage( "Add link target operation cancelled." );
        return;
    }
    if ( (targetIndex=activeGraph.hasVertex(targetNode)) ==-1 ) {
        statusMessage( tr("Aborting. ")  );
        QMessageBox::critical(this,"Error","No such node.", "OK",0);
        qDebug ("MW: slotAddLink: Cant find targetNode %i",targetNode);
        return;
    }

    weight=QInputDialog::getDouble(this, "Create new link, Step 3", tr("Source and target nodes accepted. \n Please, enter the weight of new link: "),1.0, -20.0, 20.0, 1, &ok);
    if (!ok) {
        statusMessage( "Add link operation cancelled." );
        return;
    }
    //Check if this link already exists...
    if (activeGraph.hasEdge(sourceNode, targetNode)!=0 ) {
        qDebug("Link exists. Aborting");
        statusMessage( tr("Aborting. ")  );
        QMessageBox::critical(this,"Error","Link already exists.", "OK",0);
        return;
    }

    addLink(sourceNode, targetNode, weight);
    graphChanged();
    statusMessage( tr("Ready. ")  );
}



/** 	
    helper to slotAddLink() above
    Also called from GW::userMiddleClicked() signal when user creates links with middle-clicks
    Calls Graph::createEdge method to add the new edge to the active Graph
*/
void MainWindow::addLink (int v1, int v2, float weight) {
    qDebug("MW: addLink() - setting user preferences and calling Graph::createEdge(...)");
    bool drawArrows=displayLinksArrowsAct->isChecked();
    int reciprocal=0;
    bool bezier = false;
    activeGraph.createEdge(v1, v2, weight, reciprocal, drawArrows, bezier);
}



/**
*	Erases the clicked link. Otherwise asks the user to specify one link.
*	First deletes arc reference from object nodeVector
*	then deletes arc item from scene
**/
void MainWindow::slotRemoveLink(){ 
    if ( (!fileLoaded && !networkModified) || activeGraph.totalEdges() ==0 )  {
        QMessageBox::critical(this, "Error",tr("No links present! \nLoad a network file or create a new network first."), "OK",0);
        statusMessage( tr("No links to remove - sorry.")  );
        return;
    }

    int min=0, max=0, sourceNode=-1, targetNode=-1;
    bool ok=false;
    min=activeGraph.firstVertexNumber();
    max=activeGraph.lastVertexNumber();

    if (!linkClicked) {
        sourceNode=QInputDialog::getInt(this,tr("Remove link"),tr("Source node:  (")+QString::number(min)+"..."+QString::number(max)+"):", min, 1, max , 1, &ok )   ;
        if (!ok) {
            statusMessage( "Remove link operation cancelled." );
            return;
        }

        targetNode=QInputDialog::getInt(this, tr("Remove link"), tr("Target node:  (")+QString::number(min)+"..."+QString::number(max)+"):",min, 1, max , 1, &ok )   ;
        if (!ok) {
            statusMessage( "Remove link operation cancelled." );
            return;
        }
        if ( activeGraph.hasEdge(sourceNode, targetNode)!=0 ) {
            if (activeGraph.symmetricEdge(sourceNode, targetNode) )
                graphicsWidget->unmakeEdgeReciprocal(targetNode, sourceNode);
            graphicsWidget->eraseEdge(sourceNode, targetNode);
            activeGraph.removeEdge(sourceNode, targetNode);
        }
        else {
            QMessageBox::critical(this, "Remove link",tr("There is no such link."), "OK",0);
            statusMessage( tr("There are no nodes yet...")  );
            return;
        }

    }
    else {
        sourceNode = clickedLink->sourceNodeNumber();
        targetNode = clickedLink->targetNodeNumber();
        if (activeGraph.symmetricEdge(sourceNode, targetNode) ) {
            QString s=QString::number(sourceNode);
            QString t=QString::number(targetNode);
            switch (QMessageBox::information( this, tr("Remove link"),
                                              tr("This link is reciprocal. \n") +
                                              tr("Select what Direction to delete or Both..."),
                                              s+" -> "+ t, t+" -> "+s, tr("Both"), 0, 1 ))

            {
            case 0:
                graphicsWidget->removeItem(clickedLink);
                activeGraph.removeEdge(sourceNode, targetNode);
                //make new link
                // 						graphicsWidget->unmakeEdgeReciprocal(clickedLink->targetNodeNumber(), clickedLink->sourceNodeNumber());
                //FIXME weight should be the same
                graphicsWidget->drawEdge(targetNode, sourceNode, 1, false, displayLinksArrowsAct->isChecked(), initLinkColor, false);

                break;
            case 1:
                clickedLink->unmakeReciprocal();
                //graphicsWidget->removeItem(clickedLink);
                activeGraph.removeEdge(targetNode, sourceNode);
                //						graphicsWidget->drawEdge(i, j, false, drawArrowsAct->isChecked(), initLinkColor, false);
                break;
            case 2:
                graphicsWidget->removeItem(clickedLink);
                activeGraph.removeEdge(sourceNode, targetNode);
                activeGraph.removeEdge(targetNode, sourceNode);
            }


        }
        else {
            graphicsWidget->removeItem(clickedLink);
            activeGraph.removeEdge(sourceNode, targetNode);

        }


    }
    graphChanged();
    qDebug("MW: View items now: %i ", graphicsWidget->items().size());
    qDebug("MW: Scene items now: %i ", scene->items().size());
}




/**
*	Changes the label of the clicked node
*/
void MainWindow::slotChangeNodeLabel(){
    if (!fileLoaded && !networkModified )  {
        QMessageBox::critical(this, "Error",tr("There are no nodes! \nLoad a network file or create a new network first."), "OK",0);
        statusMessage( tr("No nodes created.")  );
        return;
    }
    if (clickedJimNumber==-1) {
        statusMessage( tr("Please click on a node first... ")  );
        return;
    }
    bool ok;
    QString text = QInputDialog::getText(this, "Change node label", tr("Enter new node label:"), QLineEdit::Normal,
                                         QString::null, &ok );
    if ( ok && !text.isEmpty() ) {
        qDebug()<<"MW: change label to "<< text.toLatin1();
        clickedJim->setLabelText(text);
        activeGraph.setVertexLabel( clickedJimNumber, text);
        if (!showLabels())
            displayNodeLabelsAct->setChecked(true);
        statusMessage( tr("Changed label to %1. Ready. ").arg(text)  );
        graphChanged();
    }
    else {
        statusMessage( tr("No label text. Abort. ")  );
    }
}




/**
*	Changes the colour of the clicked node
*/
void MainWindow::slotChangeNodeColor(){
    if (!fileLoaded && !networkModified )  {
        QMessageBox::critical(this, "Error",tr("There are no nodes! \nLoad a network file or create a new network first."), "OK",0);
        statusMessage( tr("No nodes...")  );
        return;
    }

    bool ok;
    if (clickedJimNumber==-1) {
        int min=activeGraph.firstVertexNumber();
        int max=activeGraph.lastVertexNumber();
        long int node=QInputDialog::getInt(this, "Change node color",tr("Select node:  	("+QString::number(min).toLatin1()+"..."+QString::number(max).toLatin1()+"):"), min, 1, max , 1, &ok)   ;
        statusMessage( tr("Error. ")  );
        if (!ok) {
            statusMessage( "Change clicked node color operation cancelled." );
            return;
        }

        QString newColor = QInputDialog::getItem(this,"Change node color", "Select a  new color:", colorList, 1, true, &ok);
        if (!ok) {
            statusMessage( "Change clicked node color operation cancelled." );
            return;
        }

        if (graphicsWidget->setNodeColor(node, newColor)) {
            activeGraph.setVertexColor( node, newColor);
            graphChanged();
        }
        else
            statusMessage( tr("There is no such link. ") );

    }
    else{
        QString nodeColor = QInputDialog::getItem(this,"Change node color", "Select a  color:", colorList, 1, true, &ok);
        if ( ok ) {
            clickedJim->setColor(nodeColor);
            activeGraph.setVertexColor (clickedJimNumber, nodeColor);
            graphChanged();
            statusMessage( tr("Ready. ") );
        }
        else {
            // user pressed Cancel
            statusMessage( tr("Change node color aborted. ") );
        }
    }
}



/**
*	Changes the size of the clicked node.  
*/
void MainWindow::slotChangeNodeSize(){
    if (!fileLoaded && !networkModified )  {
        QMessageBox::critical(this, "Error",tr("There are no nodes! \nLoad a network file or create a new network first."), "OK",0);
        statusMessage( tr("Cannot change nothing.")  );
        return;
    }
    if (clickedJimNumber==-1) {
        statusMessage( tr("Error. ")  );
        return;
    }
    bool ok=false;
    int newSize = QInputDialog::getInt(this, "Change node size", tr("Change node size to: (1-100)"),initNodeSize, 1, 100, 1, &ok ) ;
    if (!ok) {
        statusMessage( "Change size operation cancelled." );
        return;
    }
    clickedJim->setSize(newSize);
    activeGraph.setVertexSize(clickedJimNumber,newSize);
    graphChanged();
    statusBar()->showMessage (QString(tr("Ready")), statusBarDuration) ;
    return;
}



/**
*	TODO Change the value of the clicked node.  
*/
void MainWindow::slotChangeNodeValue(){
    if (clickedJimNumber==-1) {
        statusMessage( tr("Error. ")  );
        return;
    }
    //	bool ok=false;
    //int newSize =   QInputDialog::getInt(this, "Change node value", tr("Change node size to: (1-16)"),1, 1, 16, 1, &ok ) ;
    //	clickedJim->setSize(newSize);
    graphChanged();
    statusBar()->showMessage (QString(tr("Ready")), statusBarDuration) ;
    return;
}



/**
*	Changes the shape of the clicked node to box
*/
void MainWindow::slotChangeNodeBox(){
    activeGraph.setVertexShape( clickedJim->nodeNumber(), "box");
    clickedJim->setShape("box");
    graphChanged();
}



/**
*	Changes the shape of the clicked node to triangle
*/
void MainWindow::slotChangeNodeTriangle(){
    activeGraph.setVertexShape( clickedJim->nodeNumber(), "triangle");
    clickedJim->setShape("triangle");
    graphChanged();
}



/**
*	Changes the shape of the clicked node to circle
*/
void MainWindow::slotChangeNodeCircle(){
    activeGraph.setVertexShape( clickedJim->nodeNumber(), "circle");
    clickedJim->setShape("circle");
    graphChanged();
}



/**
*	Changes the shape of the clicked node to diamond
*/
void MainWindow::slotChangeNodeDiamond(){
    activeGraph.setVertexShape( clickedJim->nodeNumber(), "diamond");
    clickedJim->setShape("diamond");
    graphChanged();
}



/**
*	Changes the shape of the clicked node to ellipse
*/
void MainWindow::slotChangeNodeEllipse(){
    activeGraph.setVertexShape( clickedJim->nodeNumber(), "ellipse");
    clickedJim->setShape("ellipse");
    graphChanged();
}


//TODO slotChangeLinkLabel
void MainWindow::slotChangeLinkLabel(){
    graphChanged();
}



/**
*	Changes the colour of the clicked link. 
*	If no link is clicked, then it asks the user to specify one.
*/
void MainWindow::slotChangeLinkColor(){
    if (!fileLoaded && !networkModified )  {
        QMessageBox::critical(this, "Error",tr("No links here! \nLoad a network file or create a new network first."), "OK",0);
        statusMessage( tr("No links present...")  );
        return;
    }

    int sourceNode=-1, targetNode=-1;
    bool ok=false;
    QString newColor;
    int min=activeGraph.firstVertexNumber();
    int max=activeGraph.lastVertexNumber();

    if (!linkClicked) {	//no edge clicked. Ask user to define an edge.
        sourceNode=QInputDialog::getInt(this, "Change link color",tr("Select link source node:  ("+QString::number(min).toLatin1()+"..."+QString::number(max).toLatin1()+"):"), min, 1, max , 1, &ok)   ;
        if (!ok) {
            statusMessage( "Change link color operation cancelled." );
            return;
        }
        targetNode=QInputDialog::getInt(this, "Change link color...", tr("Select link target node:  ("+QString::number(min).toLatin1()+"..."+QString::number(max).toLatin1()+"):"),min, 1, max , 1, &ok  )   ;
        if (!ok) {
            statusMessage( "Change link color operation cancelled." );
            return;
        }

        qDebug("source %i target %i",sourceNode, targetNode);

        newColor = QInputDialog::getItem(this , "Change link color....", "Select a  color:", colorList, 1, false, &ok );
        if ( ok ) {
            if (graphicsWidget->setEdgeColor(sourceNode, targetNode, newColor))
                activeGraph.setEdgeColor( sourceNode, targetNode, newColor);
            else
                statusMessage( tr("There is no such link. ") );
        }
        else
            statusMessage( tr("Change link color cancelled. ") );

    }
    else {	//edge has been clicked. Just ask the color and call the appropriate methods.
        newColor = QInputDialog::getItem(this, "Change link color....", "Select a new color for the clicked link:", colorList, 1, false, &ok );
        if ( ok ) {
            clickedLink->setColor(newColor);
            activeGraph.setEdgeColor( clickedLink->sourceNodeNumber(), clickedLink->targetNodeNumber(), newColor);
            statusMessage( tr("Ready. ")  );

        }
        else {       // user pressed Cancel
            statusMessage( tr("User abort. ")  );
        }
    }
}




/**
*	Changes the weight of the clicked link. 
*	If no link is clicked, asks the user to specify a link.
*/
void MainWindow::slotChangeLinkWeight(){
    if (!fileLoaded && !networkModified )  {
        QMessageBox::critical(this, "Error",tr("There are no links here! \nLoad a network file or create a new network first."), "OK",0);
        statusMessage( tr("No links present...")  );
        return;
    }

    qDebug("MW: slotChangeLinkWeight()");
    int  sourceNode=-1, targetNode=-1;
    float newWeight=1.0;
    int min=activeGraph.firstVertexNumber();
    int max=activeGraph.lastVertexNumber();

    bool ok=false;
    if (!linkClicked) {
        sourceNode=QInputDialog::getInt(this, "Change link weight",tr("Select link source node:  ("+QString::number(min).toLatin1()+"..."+QString::number(max).toLatin1()+"):"), min, 1, max , 1, &ok)   ;
        if (!ok) {
            statusMessage( "Change link weight operation cancelled." );
            return;
        }

        targetNode=QInputDialog::getInt(this, "Change link weight...", tr("Select link target node:  ("+QString::number(min).toLatin1()+"..."+QString::number(max).toLatin1()+"):"),min, 1, max , 1, &ok  )   ;
        if (!ok) {
            statusMessage( "Change link weight operation cancelled." );
            return;
        }

        qDebug("source %i target %i",sourceNode, targetNode);

        QList<QGraphicsItem *> list=scene->items();
        for (QList<QGraphicsItem *>::iterator it=list.begin(); it!= list.end() ; it++)
            if ( (*it)->type()==TypeEdge) {
                Edge *link=(Edge*) (*it);
                qDebug ("MW: searching link...");
                if ( link->sourceNodeNumber()==sourceNode && link->targetNodeNumber()==targetNode ) {
                    qDebug("MW: link found");
                    newWeight=(float) QInputDialog::getDouble(this,
                                                              "Change link weight...",tr("New link Weight: "), 1, -100, 100 ,1, &ok ) ;
                    if (ok) {
                        link->setWeight(newWeight);
                        link->update();
                        activeGraph.setEdgeWeight(sourceNode, targetNode, newWeight);
                        statusMessage(  QString(tr("Ready."))  );
                        return;
                    }
                    else {
                        statusMessage(  QString(tr("input error. Abort."))  );
                        return;
                    }
                }
            }
    }
    else {  //linkClicked
        qDebug() << "MW: slotChangeLinkWeight() - a link has already been clicked";
        sourceNode=clickedLink->sourceNodeNumber();
        targetNode=clickedLink->targetNodeNumber();
        if ( activeGraph.symmetricEdge(sourceNode, targetNode) ) {
            QString s=QString::number(sourceNode);
            QString t=QString::number(targetNode);
            switch (QMessageBox::information( this, tr("Change link weight"),
                                              tr("This link is reciprocal. \n") +
                                              tr("Select what Direction to change or Both..."),
                                              s+" -> "+ t, t+" -> "+s, tr("Both"), 0, 1 ))
            {
            case 0:
                qDebug("MW: slotChangeLinkWeight()  real edge %i -> %i", sourceNode, targetNode);
                newWeight=QInputDialog::getDouble(this,
                                                  "Change link weight...",tr("New link weight: "), 1.0, -100.0, 100.00 ,1, &ok) ;
                if (ok) {
                    clickedLink->setWeight(newWeight);
                    clickedLink->update();
                    qDebug()<<"MW: newWeight will be "<< newWeight;
                    activeGraph.setEdgeWeight(sourceNode, targetNode, newWeight);
                    statusMessage(  QString(tr("Ready."))  );
                    return;
                }
                else {
                    statusMessage(  QString(tr("Change link weight cancelled."))  );
                    return;
                }
                break;
            case 1:
                qDebug("MW: slotChangeLinkWeight() virtual edge %i -> %i",targetNode , sourceNode);
                newWeight=(float) QInputDialog::getDouble(this,
                                                          "Change link weight...",tr("New link Weight: "), 1, -100, 100 ,1, &ok ) ;
                if (ok) {
                    qDebug()<<"MW: newWeight will be "<< newWeight;
                    activeGraph.setEdgeWeight( targetNode, sourceNode, newWeight);
                    statusMessage(  QString(tr("Ready."))  );
                    return;
                }
                else {
                    statusMessage(  QString(tr("Change link weight cancelled."))  );
                    return;
                }
                break;
            case 2:
                qDebug("MW: slotChangeLinkWeight()  both directions %i <-> %i",targetNode , sourceNode);
                newWeight=(float) QInputDialog::getDouble(this,
                                                          "Change link weight...",tr("New link Weight: "), 1, -100, 100 ,1, &ok ) ;

                if (ok) {
                    qDebug()<<"MW: Changing first direction. NewWeight will be "<< newWeight;
                    activeGraph.setEdgeWeight(sourceNode, targetNode, newWeight);
                    qDebug()<<"MW: Changing opposite direction. NewWeight will be "<< newWeight;
                    activeGraph.setEdgeWeight( targetNode, sourceNode, newWeight);
                    statusMessage(  QString(tr("Ready."))  );
                    return;
                }
                else {
                    statusMessage(  QString(tr("Change link weight cancelled."))  );
                    return;
                }
                break;
            }
        }
        else {
            qDebug("MW: slotChangeLinkWeight()  real edge %i -> %i", sourceNode, targetNode);
            newWeight=QInputDialog::getDouble(this,
                                              "Change link weight...",tr("New link weight: "), 1.0, -100.0, 100.00 ,1, &ok) ;
            if (ok) {
                clickedLink->setWeight(newWeight);
                clickedLink->update();
                qDebug()<<"MW: newWeight will be "<< newWeight;
                activeGraph.setEdgeWeight(sourceNode, targetNode, newWeight);
                statusMessage(  QString(tr("Ready."))  );
                return;
            }
            else {
                statusMessage(  QString(tr("Change link weight cancelled."))  );
                return;
            }

        }
        linkClicked=false;
    }

}




/**
*	Filters Nodes by their value   
    TODO slotFilterNodes
*	
*/
void MainWindow::slotFilterNodes(){

    if (!fileLoaded && !networkModified  )  {
        QMessageBox::critical(this, "Error",tr("Nothing to filter! \nLoad a network file or create a new network. \nThen ask me to compute something!"), "OK",0);

        statusMessage(  QString(tr("Nothing to filter!"))  );
        return;
    }
}

/**
*	Calls Graph::filterOrphanVertices to filter vertices with no links
*/
void MainWindow::slotFilterOrphanNodes(){
    if (!fileLoaded && !networkModified  )  {
        QMessageBox::critical(this, "Error",tr("Nothing to filter! \nLoad a network file or create a new network. \nThen ask me to compute something!"), "OK",0);
        statusMessage(  QString(tr("Nothing to filter!"))  );
        return;
    }
    qDebug()<< "MW: slotFilterOrphanNodes";
    activeGraph.filterOrphanVertices( ! filterOrphanNodesAct->isChecked() );
}


/**
*	Shows a dialog from where the user may  
*	filter edges according to their weight 
*	All edges weighted more (or less) than the specified weight  will be disabled.
*/ 
void MainWindow::slotShowFilterEdgesDialog() {
    if (!fileLoaded && !networkModified  )   {
        statusMessage(  QString(tr("Load a network file first. \nThen you may ask me to compute something!"))  );
        return;
    }
    m_filterEdgesByWeightDialog.exec() ;
}








/**
*	Transforms all nodes to links
    TODO slotTransformNodes2Links
*/
void MainWindow::slotTransformNodes2Links(){
    graphChanged();

}





/**
*	Converts all edges to double edges, so that the network becomes undirected (symmetric adjacency matrix).
*/
void MainWindow::slotSymmetrize(){
    if (!fileLoaded && !networkModified )  {
        QMessageBox::critical(this, "Error",tr("No links here! \nLoad a network file or create a new network first."), "OK",0);
        statusMessage( tr("No links present...")  );
        return;
    }
    qDebug("MW: slotSymmetrize() calling symmetrize");
    activeGraph.symmetrize();
    QMessageBox::information(this, "Symmetrize",tr("All links are reciprocal. \nYour network is symmetric..."), "OK",0);
    statusBar()->showMessage (QString(tr("Ready")), statusBarDuration) ;
}



/**
    TODO slotColorationStrongStructural
*/
void MainWindow::slotColorationStrongStructural() {
}


/**
    TODO slotColorationRegular
*/
void MainWindow::slotColorationRegular() {
}



/**
*	Calls Graph::LayoutRadialCentrality()
*	to reposition all nodes on a circular layout based on their In-Degree Centralities. 
*	More central nodes are closer to the centre
*/
void MainWindow::slotLayoutRandom(){
    if (!fileLoaded && !networkModified  )  {
        QMessageBox::critical(this, "Error",tr("Sorry, I can't follow! \nLoad a network file or create a new network first. \nThen we can talk about layouts!"), "OK",0);
        statusMessage(  QString(tr("Nothing to layout! Are you dreaming?"))  );
        return;
    }
    double maxWidth=graphicsWidget->width();
    double maxHeight=graphicsWidget->height();
    statusMessage(  QString(tr("Randomizing nodes positions. Please wait...")) );
    graphicsWidget->clearGuides();
    createProgressBar();
    activeGraph.layoutRandom(maxWidth, maxHeight);
    destroyProgressBar();
    statusMessage( tr("Node positions are now randomized.") );
}


/** 
    TODO slotLayoutRandomCircle
*/
void MainWindow::slotLayoutRandomCircle(){
}





/**
        slotLayoutSpringEmbedder called from menu or toolbox checkbox
*/
void MainWindow::slotLayoutSpringEmbedder(bool state ){
    qDebug()<< "MW:slotLayoutSpringEmbedder";
    if (!fileLoaded && !networkModified  )  {
        QMessageBox::critical(this, "Error",tr("There are node nodes yet!\nLoad a network file or create a new network first. \nThen we can talk about layouts!"), "OK",0);
        statusMessage( tr("I am really sorry. You must really load a file first... ")  );
        moveSpringEmbedderBx->setCheckState(Qt::Unchecked);
        return;
    }

    //Stop any other layout running
    moveFruchtermanBx->setCheckState(Qt::Unchecked);
    activeGraph.nodeMovement(!state, 2, graphicsWidget->width(), graphicsWidget->height());

    scene->setItemIndexMethod (QGraphicsScene::NoIndex); //best when moving items

    if (state){
        statusMessage( tr("Embedding a spring-gravitational model on the network.... ")  );
        moveSpringEmbedderBx->setCheckState(Qt::Checked);
        activeGraph.nodeMovement(state, 1, graphicsWidget->width(), graphicsWidget->height());
        statusMessage( tr("Click on the checkbox \"Spring-Embedder\" to stop movement!") );
    }
    else {
        moveSpringEmbedderBx->setCheckState(Qt::Unchecked);
        activeGraph.nodeMovement(state, 1, graphicsWidget->width(), graphicsWidget->height());
        statusMessage( tr("Movement stopped!") );
    }
    scene->setItemIndexMethod (QGraphicsScene::BspTreeIndex); //best when not moving items
}





/**
    slotLayoutFruchterman called from menu
*/
void MainWindow::slotLayoutFruchterman(){
    if (!fileLoaded && !networkModified  )  {
        QMessageBox::critical(this, "Error",tr("There are no nodes yet!\nLoad a network file or create a new network first. \nThen we can talk about layouts!"), "OK",0);
        statusMessage( tr("I am really sorry. You must really load a file first... ")  );
        return;
    }
    if (moveFruchtermanBx->checkState() == Qt::Unchecked){
        statusMessage( tr("Embedding a repelling-attracting forces model on the network.... ")  );
        moveFruchtermanBx->setCheckState(Qt::Checked);
        statusMessage( tr("Click on the checkbox \"Fruchterman-Reingold\" to stop movement!") );
    }
    else {
        moveFruchtermanBx->setCheckState(Qt::Unchecked);
        statusMessage( tr("Movement stopped!") );
    }

}


/** 
        Called from button.
    Calls Graph::startNodeMovement to embed a repelling-attracting forces layout...
*/
void MainWindow::layoutFruchterman (int state){
    qDebug("MW: layoutFruchterman ()");
    moveSpringEmbedderBx->setChecked(false);
    scene->setItemIndexMethod (QGraphicsScene::NoIndex); //best when moving items
    activeGraph.nodeMovement(state, 2, graphicsWidget->width(), graphicsWidget->height());
    scene->setItemIndexMethod (QGraphicsScene::BspTreeIndex); //best when not moving items
}


/** 
    Resizes all nodes according to the amount of their out-Links from other nodes.
*/
void MainWindow::slotLayoutNodeSizeProportionalOutEdges(bool checked){
    if (!fileLoaded && !networkModified  )  {
        QMessageBox::critical(this, "Error",tr("Wake up! \nLoad a network file or create a new network first. \nThen we can talk about layouts!"), "OK",0);
        statusMessage( tr("I am really sorry. You must really load a file first... ")  );
        return;
    }

    qDebug("MW: slotLayoutNodeSizeProportionalOutEdges()");
    QList<QGraphicsItem *> list=scene->items();
    int edges = 0, size = initNodeSize ;

    if (checked != true) {
        QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
        for (QList<QGraphicsItem *>::iterator it=list.begin(); it!=list.end(); it++)  {
            if ( (*it) -> type() == TypeNode ){
                Node *jim = (Node*) (*it);
                (*jim).setSize(size);
            }
        }

        nodeSizeProportionalOutDegreeAct->setChecked(false);
        nodeSizeProportional2OutDegreeBx->setChecked(false);
        QApplication::restoreOverrideCursor();
        return;
    }
    nodeSizeProportionalOutDegreeAct->setChecked(true);
    nodeSizeProportional2OutDegreeBx->setChecked(true);
    statusMessage( tr("Embedding node size model on the network.... ")  );
    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
    for (QList<QGraphicsItem *>::iterator it=list.begin(); it!=list.end(); it++) {
        if ( (*it) -> type() == TypeNode ){
            Node *jim = (Node*) (*it);
            edges = activeGraph.outEdges(  (*jim).nodeNumber() ) ;
            qDebug() << "Node " << (*jim).nodeNumber() <<  " outDegree:  "<<  edges;

            if (edges == 0 ) {
                size = initNodeSize;
            }
            else if (edges == 1 ) {
                size = initNodeSize + 1;
            }
            else if (edges > 1 && edges <= 2  ) {
                size = initNodeSize + 2;
            }
            else if (edges > 2 && edges <= 3  ) {
                size = initNodeSize + 3;
            }
            else if (edges > 3 && edges <= 4  ) {
                size = initNodeSize + 4;
            }
            else if (edges > 4 && edges <= 6  ) {
                size = initNodeSize + 4;
            }
            else if (edges > 5 && edges <= 7  ) {
                size = initNodeSize+5 ;
            }
            else if (edges > 7 && edges <= 10  ) {
                size = initNodeSize+6 ;
            }
            else if (edges > 10 && edges <= 15  ) {
                size = initNodeSize+7 ;
            }
            else if (edges > 15 && edges <= 25  ) {
                size = initNodeSize+8 ;
            }
            else  if (edges > 25 ) {
                size = initNodeSize+9;
            }
            qDebug() << "Changing size of " << (*jim).nodeNumber()  << "  to " <<  size;
            (*jim).setSize(size);
        }

    }
    QApplication::restoreOverrideCursor( );

}







/** 
    Resizes all nodes according to the amount of their in-Links from other nodes.
*/
void MainWindow::slotLayoutNodeSizeProportionalInEdges(bool checked){
    if (!fileLoaded && !networkModified  )  {
        QMessageBox::critical(this, "Error",tr("You must be dreaming! \nLoad a network file or create a new network first. \nThen we can talk about layouts!"), "OK",0);
        statusMessage( tr("I am really sorry. You must really load a file first... ")  );
        return;
    }

    qDebug("MW: slotLayoutNodeSizeProportionalInEdges()");
    QList<QGraphicsItem *> list=scene->items();
    int edges = 0, size = initNodeSize ;

    if (checked != true) {
        for (QList<QGraphicsItem *>::iterator it=list.begin(); it!=list.end(); it++)  {
            if ( (*it) -> type() == TypeNode ){
                Node *jim = (Node*) (*it);
                (*jim).setSize(size);
            }
        }
        nodeSizeProportionalInDegreeAct->setChecked(false);
        nodeSizeProportional2InDegreeBx->setChecked(false);
        return;
    }
    nodeSizeProportionalInDegreeAct->setChecked(true);
    nodeSizeProportional2InDegreeBx->setChecked(true);
    statusMessage( tr("Embedding node size model on the network.... ")  );
    for (QList<QGraphicsItem *>::iterator it=list.begin(); it!=list.end(); it++) {
        if ( (*it) -> type() == TypeNode ){
            Node *jim = (Node*) (*it);
            edges = activeGraph.inEdges(  (*jim).nodeNumber() ) ;
            qDebug() << "Node " << (*jim).nodeNumber() << " inDegree:  " <<  edges;

            if (edges == 0 ) {
                size = initNodeSize;
            }
            else if (edges == 1 ) {
                size = initNodeSize + 1;
            }
            else if (edges > 1 && edges <= 2  ) {
                size = initNodeSize + 2;
            }
            else if (edges > 2 && edges <= 3  ) {
                size = initNodeSize + 3;
            }
            else if (edges > 3 && edges <= 4  ) {
                size = initNodeSize + 4;
            }
            else if (edges > 4 && edges <= 6  ) {
                size = initNodeSize + 4;
            }
            else if (edges > 5 && edges <= 7  ) {
                size = initNodeSize+5 ;
            }
            else if (edges > 7 && edges <= 10  ) {
                size = initNodeSize+6 ;
            }
            else if (edges > 10 && edges <= 15  ) {
                size = initNodeSize+7 ;
            }
            else if (edges > 15 && edges <= 25  ) {
                size = initNodeSize+8 ;
            }
            else  if (edges > 25 ) {
                size = initNodeSize+9;
            }
            qDebug() << "Changing size of " <<  (*jim).nodeNumber() << " to " <<  size;
            (*jim).setSize(size);
        }

    }

}


/**
 * @brief MainWindow::slotLayoutRadialByProminenceIndex
 * Checks sender text() to find out who QMenu item was pressed
 * and what index was chosen
 * then repositions all nodes  on a radial layout based on that index
*  More prominent nodes are closer to the centre of the screen.
 */
void MainWindow::slotLayoutRadialByProminenceIndex(){
    if (!fileLoaded && !networkModified  )  {
        QMessageBox::critical(this, "Error",tr("Sorry, I can't follow! \nLoad a network file or create a new network first. \nThen we can talk about layouts!"), "OK",0);
        statusMessage(  QString(tr("Nothing to layout! Are you dreaming?"))  );
        return;
    }
    QAction *menuitem=(QAction *) sender();
    qDebug() << "MainWindow::slotLayoutRadialByProminenceIndex() - " <<
                "SENDER MENU IS " << menuitem->text();
    double x0=scene->width()/2.0;
    double y0=scene->height()/2.0;
    double maxRadius=(graphicsWidget->height()/2.0)-50;          //pixels
    int userChoice = 0;
    statusMessage(  QString(tr("Calculating new nodes positions. Please wait...")) );
    graphicsWidget->clearGuides();
    createProgressBar();
    if (menuitem->text() == "Degree Centrality")
        userChoice=1;
    else if (menuitem->text() == "Closeness Centrality")
        userChoice=2;
    else if (menuitem->text() == "Influence Range Closeness Centrality")
        userChoice=3;
    else if (menuitem->text() == "Betweeness Centrality")
        userChoice=4;
    else if (menuitem->text() == "Stress Centrality")
        userChoice=5;
    else if (menuitem->text() == "Eccentricity Centrality")
        userChoice=6;
    else if (menuitem->text() == "Power Centrality")
        userChoice=7;
    else if (menuitem->text() ==  "Information Centrality")
        userChoice=8;
    else if (menuitem->text() == "Degree Prestige")
        userChoice=9;
    else if (menuitem->text() ==  "PageRank Prestige")
        userChoice=10;
    else if (menuitem->text() ==  "Proximity Prestige")
        userChoice=11;

    activeGraph.layoutRadialByProminenceIndex(x0, y0, maxRadius,userChoice);
    destroyProgressBar();
    statusMessage( tr("Nodes in inner circles have greater prominence index.") );
}


/**
*	Calls Graph::layoutLayeredCentrality 
*	to reposition all nodes on different top-down levels according to their 
*	In-Degree Centrality
*	More central nodes are closer to the top of the canvas
*/
void MainWindow::slotLayoutLayeredCentralityInDegree(){
    if (!fileLoaded && !networkModified  )  {
        QMessageBox::critical(this, "Error",tr("Nothing to do!\nLoad a network file or create a new network first. \nThen we can talk about layouts!"), "OK",0);
        statusMessage(  QString(tr("Nothing to layout! Are you dreaming?"))  );
        return;
    }
    double maxWidth=scene->width();
    double maxHeight=scene->height(); //pixels

    statusMessage(  QString(tr("Calculating new nodes positions. Please wait...")) );
    graphicsWidget->clearGuides();
    createProgressBar();
    activeGraph.layoutLayeredCentrality(maxWidth, maxHeight, 1);
    destroyProgressBar();
    statusMessage( tr("Nodes in upper levels have greater In-Degree Centrality. ") );

}



/**
*	Calls Graph::layoutLayeredCentrality 
*	to reposition all nodes on different top-down levels according to their 
*	Out-Degree Centrality
*	More central nodes are closer to the top of the canvas
*/
void MainWindow::slotLayoutLayeredCentralityOutDegree(){
    if (!fileLoaded && !networkModified  )  {
        QMessageBox::critical(this, "Error",tr("Load a network file or create a new network first. \nThen we can talk about layouts!"), "OK",0);
        statusMessage(  QString(tr("Nothing to layout! Are you dreaming?"))  );
        return;
    }
    double maxWidth=scene->width();
    double maxHeight=scene->height(); //pixels

    statusMessage(  QString(tr("Calculating new nodes positions. Please wait...")) );
    graphicsWidget->clearGuides();
    createProgressBar();
    activeGraph.layoutLayeredCentrality(maxWidth, maxHeight, 2);
    destroyProgressBar();
    statusMessage( tr("Nodes in upper levels have greater Out-Degree Centrality. ") );

}


/**
*	Calls Graph::LayoutLayeredCentrality 
*	to reposition all nodes on different top-down levels according to their 
*	Closeness Centrality
*	More central nodes are closer to the top of the canvas

*/
void MainWindow::slotLayoutLayeredCentralityCloseness(){
    if (!fileLoaded && !networkModified  )  {
        QMessageBox::critical(this, "Error",tr("Load a network file or create a new network first. \nThen we can talk about layouts!"), "OK",0);
        statusMessage(  QString(tr("Nothing to layout! Are you dreaming?"))  );
        return;
    }
    double maxWidth=scene->width();
    double maxHeight=scene->height(); //pixels

    statusMessage(  QString(tr("Calculating new nodes positions. Please wait..."))  );
    graphicsWidget->clearGuides();
    createProgressBar();
    activeGraph.layoutLayeredCentrality(maxWidth, maxHeight, 3);
    destroyProgressBar();

    statusMessage( tr("Nodes in upper levels have greater Closeness Centrality.") );
}



/**
*	Calls Graph::LayoutLayeredCentrality 
*	to reposition all nodes on different top-down levels according to their 
*	Betweeness Centrality
*	More central nodes are closer to the top of the canvas
*/
void MainWindow::slotLayoutLayeredCentralityBetweeness(){
    if (!fileLoaded && !networkModified  )  {
        QMessageBox::critical(this, "Error",tr("Nothing to do!\nLoad a network file or create a new network first. \nThen we can talk about layouts!"), "OK",0);
        statusMessage(  QString(tr("Nothing to layout! Are you dreaming?"))  );
        return;
    }
    double maxWidth=scene->width();
    double maxHeight=scene->height(); //pixels

    statusMessage(  QString(tr("Calculating new nodes positions. Please wait...")) );
    graphicsWidget->clearGuides();
    createProgressBar();
    activeGraph.layoutLayeredCentrality(maxWidth, maxHeight, 4);
    destroyProgressBar();
    statusMessage( tr("Nodes in upper levels have greater Betweeness Centrality. ") );
}


/**
    TODO slotLayoutLayeredCentralityInformational
*/
void MainWindow::slotLayoutLayeredCentralityInformation(){
}







/**
*	Returns the amount of active links on the scene.
*/
int MainWindow::activeLinks(){
    qDebug ("activeLinks()");
    totalLinks=activeGraph.totalEdges();
    return totalLinks;
}








/**
*	Returns the amount of active nodes on the scene.
*/
int MainWindow::activeNodes(){ 
    return activeGraph.vertices();
}







/**
*	Displays a box informing the user about the symmetry or not of the adjacency matrix
*/

void MainWindow::slotCheckSymmetry(){
    if (!fileLoaded && !networkModified  )   {
        QMessageBox::critical(this, "Error",tr("There are no nodes!\nLoad a network file or create a new network. \nThen ask me to compute something!"), "OK",0);
        statusMessage(  QString(tr("There is no network!"))  );
        return;
    }
    if (activeGraph.isSymmetric())
        QMessageBox::information(this,
                                 "Symmetry",
                                 tr("The adjacency matrix is symmetric."
                                    ),"OK",0);
    else
        QMessageBox::information(this,
                                 "Symmetry",
                                 tr("The adjacency matrix is not symmetric."
                                    ),"OK",0);

    statusMessage (QString(tr("Ready")) );

}


void MainWindow::slotInvertAdjMatrix(){
    if ( !fileLoaded && !networkModified) {
        QMessageBox::critical (this, "Error",
                               tr("Empty network! \nLoad a network file or create something by double-clicking on the canvas!"), "OK",0);

        statusMessage(  tr("Nothing to show!") );
        return;
    }
    int aNodes=activeNodes();
    statusBar() ->  showMessage ( QString (tr ("inverting adjacency adjacency matrix of %1 nodes")).arg(aNodes) );
    qDebug ("MW: calling Graph::writeInvertAdjacencyMatrix with %i nodes", aNodes);
    char fn[]= "invert-adjacency-matrix.dat";

    activeGraph.writeInvertAdjacencyMatrix(fn, networkName.toLocal8Bit()) ;

    //Open a text editor window for the new file created by graph class
    QString qfn=QString::fromLocal8Bit("invert-adjacency-matrix.dat");
    TextEditor *ed = new TextEditor(fn);
    tempFileNameNoPath=qfn.split( "/");
    ed->setWindowTitle(tr("View Adjacency Matrix - ") + tempFileNameNoPath.last());
    ed->show();

}



/**
*  Displays the graph distance (geodesic distance) between two user-specified nodes
    This is the length of the shortest path between them.
*/
void MainWindow::slotGraphDistance(){
    if (!fileLoaded && !networkModified  )  {
        QMessageBox::critical(this, "Error",tr("There are no nodes!\nLoad a network file or create a new network. \nThen ask me to compute something!"), "OK",0);
        statusMessage(  QString(tr("There are no nodes. Nothing to do..."))  );
        return;
    }
    bool ok=false;
    long int  min=1, max=1, i=-1, j=-1;
    QList<QGraphicsItem *> list=scene->items();
    for (QList<QGraphicsItem *> ::iterator it=list.begin(); it!=list.end(); it++) {
        if ( (*it) -> type() == TypeNode ){
            Node *jim = (Node*) (*it);
            if ( min>jim->nodeNumber() ) min=jim->nodeNumber();
            if ( max<jim->nodeNumber() ) max=jim->nodeNumber();
        }
    }
    i=QInputDialog::getInt(this, tr("Distance between two nodes"),tr("Select source node:  ("+QString::number(min).toLatin1()+"..."+QString::number(max).toLatin1()+"):"), min, 1, max , 1, &ok )   ;
    if (!ok) {
        statusMessage( "Distance calculation operation cancelled." );
        return;
    }

    j=QInputDialog::getInt(this, tr("Distance between two nodes"), tr("Select target node:  ("+QString::number(min).toLatin1()+"..."+QString::number(max).toLatin1()+"):"),min, 1, max , 1, &ok )   ;
    if (!ok) {
        statusMessage( tr("Distance calculation operation cancelled.") );
        return;
    }

    qDebug() << "source " << i  << " target" <<  j;

    if (activeGraph.isSymmetric() && i>j) {
        qSwap(i,j);
    }
    if ( activeGraph.distance(i,j) > 0 )
        QMessageBox::information(this, tr("Distance"), tr("Network distance (")+QString::number(i)+", "+QString::number(j)+") = "+QString::number(activeGraph.distance(i,j))+tr("\nThe nodes are connected."),"OK",0);
    else
        QMessageBox::information(this, tr("Distance"), tr("Network distance (")+QString::number(i)+", "+QString::number(j)+") = "+QString::number(activeGraph.distance(i,j))+tr("\nThe nodes are not connected."),"OK",0);
}




/**
*  Invokes calculation of the matrix of geodesic distances for the loaded network, then displays it.
*/
void MainWindow::slotViewDistanceMatrix(){
    qDebug("MW: slotViewDistanceMatrix()");
    if (!fileLoaded && !networkModified  )  {
        QMessageBox::critical(this, "Error",tr("There are no nodes nor links!\nLoad a network file or create a new network. \nThen ask me to compute something!"), "OK",0);
        statusMessage(  QString(tr("Nothing to do!"))  );
        return;
    }
    statusMessage( tr("Creating distance matrix. Please wait...") );
    char fn[]= "distance-matrix.dat";

    createProgressBar();

    activeGraph.writeDistanceMatrix(fn, networkName.toLocal8Bit());

    destroyProgressBar();

    //Open a text editor window for the new file created by graph class
    TextEditor *ed = new TextEditor(fn);

    ed->setWindowTitle(tr("Matrix of geodesic distances "));
    ed->show();
}




/**
*  Invokes calculation of the sigmas matrix (the number of geodesic paths between each pair of nodes in the loaded network), then displays it.
*/
void MainWindow::slotViewNumberOfGeodesicsMatrix(){
    qDebug("MW: slotViewNumberOfGeodesics()");
    if (!fileLoaded && !networkModified  )  {
        QMessageBox::critical(this, "Error",tr("There are no nodes nor links!\nLoad a network file or create a new network. \nThen ask me to compute something!"), "OK",0);
        statusMessage(  QString(tr("Nothing to do!"))  );
        return;
    }
    statusMessage( tr("Creating number of geodesics matrix. Please wait...") );
    char fn[]="sigmas-matrix.dat";

    createProgressBar();

    activeGraph.writeNumberOfGeodesicsMatrix(fn, networkName.toLocal8Bit());

    destroyProgressBar();

    //Open a text editor window for the new file created by graph class
    TextEditor *ed = new TextEditor(fn);

    ed->setWindowTitle(tr("Matrix of sigmas (number of geodesic paths)"));
    ed->show();
}



/**  Displays the network diameter (largest geodesic) */
void MainWindow::slotDiameter() {
    if (!fileLoaded && !networkModified  )  {
        QMessageBox::critical(this, "Error",tr("There are no nodes nor links!\nLoad a network file or create a new network. \nThen ask me to compute something!"), "OK",0);

        statusMessage(  QString(tr("Cannot find the diameter of nothing..."))  );
        return;
    }

    createProgressBar();

    int netDiameter=activeGraph.diameter();

    destroyProgressBar();

    if (netDiameter > (activeGraph.vertices()-1) )
        QMessageBox::information(this, "Diameter", "Network diameter = "+ QString::number(netDiameter)+"  > (vertices()-1).", "OK",0);
    else
        QMessageBox::information(this, "Diameter", "Network diameter = " + QString::number(netDiameter), "OK",0);
    statusMessage( tr("Diameter calculated. Ready.") );

}





/**  Displays the  average shortest path length (average graph distance) */
void MainWindow::slotAverageGraphDistance() {
    if (!fileLoaded && !networkModified  )  {
        QMessageBox::critical(this, "Error",tr("There are no nodes nor links!\nLoad a network file or create a new network. \nThen ask me to compute something!"), "OK",0);

        statusMessage(  QString(tr("Cannot find the diameter of nothing..."))  );
        return;
    }

    createProgressBar();

    float averGraphDistance=activeGraph.averageGraphDistance();

    destroyProgressBar();

    QMessageBox::information(this, "Average Graph Distance", "The average shortest path length is  = " + QString::number(averGraphDistance), "OK",0);
    statusMessage( tr("Average distance calculated. Ready.") );

}


/**
*	Writes Eccentricity indices into a file, then displays it.
*/
void MainWindow::slotEccentricity(){
    if (!fileLoaded && !networkModified  )  {
        QMessageBox::critical(this, "Error",tr("There are no nodes!\nLoad a network file or create a new network. \nThen ask me to compute something!"), "OK",0);

        statusMessage(  QString(tr(" Nothing to do..."))  );
        return;
    }
    QString fn = "eccentricity.dat";
    bool considerWeights=true;
    statusMessage(  QString(tr(" Please wait...")));

    createProgressBar();
    activeGraph.writeEccentricity(fn, considerWeights);
    destroyProgressBar();

    TextEditor *ed = new TextEditor(fn);        //OPEN A TEXT EDITOR WINDOW
    tempFileNameNoPath=fn.split( "/");
    ed->setWindowTitle(tr("Eccentricity report saved as: ") + tempFileNameNoPath.last());
    ed->show();
    QApplication::restoreOverrideCursor();

}





/**
 * @brief MainWindow::slotConnectedness
 */
void MainWindow::slotConnectedness(){
    if (!fileLoaded && !networkModified  )  {
        QMessageBox::critical(this, "Error",tr("There are no nodes nor links!\nLoad a network file or create a new network. \nThen ask me to compute something!"), "OK",0);

        statusMessage(  QString(tr("Nothing to do..."))  );
        return;
    }

    createProgressBar();

    int connectedness=activeGraph.connectedness();

    destroyProgressBar();

    switch ( connectedness ) {
    case 1:
        QMessageBox::information(this, "Connectedness", "The graph representing "
                                 "the loaded network is connected.", "OK",0);
        break;
    case 0:
    QMessageBox::information(this, "Connectedness", "The digraph representing "
                            "the loaded network is weakly connected.", "OK",0);
    break;

    case -1:
    QMessageBox::information(this, "Connectedness", "The graph representing "
                             "the loaded network is disconnected.", "OK",0);
    break;
    default:
        QMessageBox::critical(this, "Connectedness", "Something went wrong!.", "OK",0);
        break;
    };
    statusMessage( tr("Connectedness calculated. Ready.") );

}


/**
*	Calls Graph:: writeNumberOfWalks() to calculate and print
*   the number of walks of a given length , between each pair of nodes.
*/
void MainWindow::slotNumberOfWalks(){
    if (!fileLoaded && !networkModified  )  {
        QMessageBox::critical(this, "Error",tr("Nothing to do! \nLoad a network file or create a new network. \nThen ask me to compute something!"), "OK",0);
        statusMessage(  QString(tr(" No network here. Sorry. Nothing to do."))  );
        return;
    }

    QString fn = "number-of-walks.dat";
     bool ok=false;
    createProgressBar();

    int length = QInputDialog::getInt(this, "Number of walks", tr("Select desired length of walk: (2 to %1)").arg(activeNodes()-1),2, 2, activeNodes()-1, 1, &ok );
    if (!ok) {
        statusMessage( "Cancelled." );
        return;
    }

    activeGraph.writeNumberOfWalksMatrix(fn, networkName, length);

    destroyProgressBar();

    TextEditor *ed = new TextEditor(fn);        //OPEN A TEXT EDITOR WINDOW
    tempFileNameNoPath=fn.split( "/");
    ed->setWindowTitle("Number of walks saved as: " + tempFileNameNoPath.last());
    ed->show();

}



/**
*	Calls Graph:: writeTotalNumberOfWalksMatrix() to calculate and print
*   the total number of walks of any length , between each pair of nodes.
*/
void MainWindow::slotTotalNumberOfWalks(){
    if (!fileLoaded && !networkModified  )  {
        QMessageBox::critical(this, "Error",tr("Nothing to do! \nLoad a network file or create a new network. \nThen ask me to compute something!"), "OK",0);
        statusMessage(  QString(tr(" No network here. Sorry. Nothing to do."))  );
        return;
    }
    if (activeNodes() > 50) {
        switch( QMessageBox::critical(this, "Slow function warning",tr("Please note that this function is VERY SLOW on large networks (n>50), since it will calculate all powers of the sociomatrix up to n-1 in order to find out all possible walks. \n\nIf you need to make a simple reachability test, we advise to use the Reachability Matrix function instead. \n\n Are you sure you want to continue?"), QMessageBox::Ok|QMessageBox::Cancel,QMessageBox::Cancel) ) {
        case QMessageBox::Ok:
            break;

        case QMessageBox::Cancel:
            // Cancel was clicked
            return;
            break;
        default:
            // should never be reached
            break;
        }
    }
    QString fn = "total-number-of-walks.dat";
    createProgressBar();

    int maxLength=activeNodes()-1;
    activeGraph.writeTotalNumberOfWalksMatrix(fn, networkName, maxLength);

    destroyProgressBar();

    TextEditor *ed = new TextEditor(fn);        //OPEN A TEXT EDITOR WINDOW
    tempFileNameNoPath=fn.split( "/");
    ed->setWindowTitle("Total number of walks saved as: " + tempFileNameNoPath.last());
    ed->show();

}



/**
*	Calls Graph:: writeReachabilityMatrix() to calculate and print
*   the Reachability Matrix of the network.
*/
void MainWindow::slotReachabilityMatrix(){
    if (!fileLoaded && !networkModified  )  {
        QMessageBox::critical(this, "Error",tr("Nothing to do! \nLoad a network file or create a new network. \nThen ask me to compute something!"), "OK",0);
        statusMessage(  QString(tr(" No network here. Sorry. Nothing to do."))  );
        return;
    }

    QString fn = "reachability-matrix.dat";

    createProgressBar();

    activeGraph.writeReachabilityMatrix(fn, networkName);

    destroyProgressBar();

    TextEditor *ed = new TextEditor(fn);        //OPEN A TEXT EDITOR WINDOW
    tempFileNameNoPath=fn.split( "/");
    ed->setWindowTitle("Reachability Matrix saved as: " + tempFileNameNoPath.last());
    ed->show();

}

/**
*	Calls Graph:: writeNumberOfCliques() to write the number of cliques (triangles)
*  of each vertex into a file, then displays it.
*/
void MainWindow::slotNumberOfCliques(){
    if (!fileLoaded && !networkModified  )  {
        QMessageBox::critical(this, "Error",tr("Nothing to do! \nLoad a network file or create a new network. \nThen ask me to compute something!"), "OK",0);
        statusMessage(  QString(tr(" No network here. Sorry. Nothing to do."))  );
        return;
    }
    QString fn = "number-of-cliques.dat";
    bool considerWeights=true;

    createProgressBar();

    activeGraph.writeNumberOfCliques(fn, considerWeights);

    destroyProgressBar();

    TextEditor *ed = new TextEditor(fn);        //OPEN A TEXT EDITOR WINDOW
    tempFileNameNoPath=fn.split( "/");
    ed->setWindowTitle("Number of cliques saved as: " + tempFileNameNoPath.last());
    ed->show();
}






/**
*	Writes Clustering Coefficients into a file, then displays it.
*/
void MainWindow::slotClusteringCoefficient (){
    if (!fileLoaded && !networkModified  )  {
        QMessageBox::critical(this, "Error",tr("Nothing to do! \nLoad a network file or create a new network. \nThen ask me to compute something!"), "OK",0);
        statusMessage(  QString(tr(" No network here. Sorry. Nothing to do."))  );
        return;
    }
    QString fn = "clustering-coefficients.dat";
    bool considerWeights=true;

    createProgressBar();

    activeGraph.writeClusteringCoefficient(fn, considerWeights);

    destroyProgressBar();

    TextEditor *ed = new TextEditor(fn);        //OPEN A TEXT EDITOR WINDOW
    tempFileNameNoPath=fn.split( "/");
    ed->setWindowTitle("Clustering Coefficients saved as: " + tempFileNameNoPath.last());
    ed->show();
}




/**
*	Calls Graph to conduct and write a triad census into a file, then displays it.
*/
void MainWindow::slotTriadCensus() {

    if (!fileLoaded && !networkModified  )  {
        QMessageBox::critical(this, "Error",tr("Nothing to do! \nLoad a network file or create a new network. \nThen ask me to compute something!"), "OK",0);
        statusMessage(  QString(tr(" No network here. Sorry. Nothing to do."))  );
        return;
    }
    QString fn = "triad-census.dat";
    bool considerWeights=true;

    createProgressBar();

    activeGraph.writeTriadCensus(fn, considerWeights);

    destroyProgressBar();

    TextEditor *ed = new TextEditor(fn);        //OPEN A TEXT EDITOR WINDOW
    tempFileNameNoPath=fn.split( "/");
    ed->setWindowTitle("Triad Census saved as: " + tempFileNameNoPath.last());
    ed->show();
}


/**
*	Writes Out-Degree Centralities into a file, then displays it.
*/
void MainWindow::slotCentralityDegree(){
    if (!fileLoaded && !networkModified  )  {
        QMessageBox::critical(this, "Error",tr("Nothing to do! \nLoad a network file or create a new network. \nThen ask me to compute something!"), "OK",0);
        statusMessage(  QString(tr(" No network here. Sorry. Nothing to do."))  );
        return;
    }
    bool considerWeights=false;
    if ( activeGraph.isWeighted()) {
        switch( QMessageBox::information( this, "Centrality Out-Degree",
                                          tr("Graph edges have weights. \nTake weights into account (Default: No)?"),
                                          tr("Yes"), tr("No"),
                                          0, 1 ) )
        {
        case 0:
            considerWeights=true;
            break;
        case 1:
            considerWeights=false;
            break;
        default: // just for sanity
            considerWeights=false;
            return;
            break;
        }

    }
    QString fn = "centrality-out-degree.dat";

    createProgressBar();

    activeGraph.writeCentralityDegree(fn, considerWeights);

    destroyProgressBar();

    statusMessage( QString(tr(" displaying file...")));

    TextEditor *ed = new TextEditor(fn);        //OPEN A TEXT EDITOR WINDOW
    tempFileNameNoPath=fn.split( "/");
    ed->setWindowTitle("Out-Degree Centralities saved as: " + tempFileNameNoPath.last());
    ed->show();
}




/**
*	Writes Closeness Centralities into a file, then displays it.
*/
void MainWindow::slotCentralityCloseness(){
    if (!fileLoaded && !networkModified  )  {
        QMessageBox::critical(this, "Error",tr("There are no nodes!\nLoad a network"
                                               " file or create a new network manually. "
                                               "\nThen ask me to compute something!"), "OK",0);
        statusMessage(  QString(tr("Nothing to do..."))  );
        return;
    }
    int connectedness=activeGraph.connectedness();

    switch ( connectedness ) {
    case 1:
        break;
    case 0:
        QMessageBox::critical(this,
                              "Centrality Closeness",
                              tr(
                                 "Weakly connected digraph!\n"
                                 "Since this network is directed and weakly "
                                 "connected, the ordinary Closeness Centrality "
                                 "index is not defined, because d(u,v) will be "
                                  "infinite for not reachable nodes u,v.\n"
                                 "Please use the slightly different but improved "
                                 "Influence Range Closeness index "
                                 "which considers how proximate is each node "
                                 "to the nodes in its influence range. \n"
                                 "Read more in the SocNetV manual."
                                 ), "OK",0);
        return;
    break;

    case -1:
        QMessageBox::critical(this,
                              "Centrality Closeness",
                              tr(
                                 "Disconnected graph/digraph!\n"
                                 "Since this network is disconnected, "
                                 "the ordinary Closeness Centrality "
                                 "index is not defined, because d(u,v) will be "
                                 "infinite for any isolate nodes u or v.\n"
                                 "Please use the slightly different but improved "
                                 "Influence Range Closeness index "
                                 "which considers how proximate is each node "
                                 "to the nodes in its influence range.\n"
                                  "Read more in the SocNetV manual."
                                 ), "OK",0);
        return;
    break;
    default:
        QMessageBox::critical(this, "Connectedness", "Something went wrong!.", "OK",0);
        break;
    };

    QString fn = "centrality_closeness.dat";
    bool considerWeights=true;

    createProgressBar();

    activeGraph.writeCentralityCloseness(fn, considerWeights);

    destroyProgressBar();

    statusMessage( QString(tr(" displaying file...")));

    TextEditor *ed = new TextEditor(fn);        //OPEN A TEXT EDITOR WINDOW
    tempFileNameNoPath=fn.split( "/");
    ed->setWindowTitle("Closeness Centralities  saved as: " + tempFileNameNoPath.last());
    ed->show();
}




/**
 * @brief MainWindow::slotCentralityClosenessInfluenceRange
*	Writes Centrality Closeness (based on Influence Range) indices into a file,
*   then displays it.
 */
void MainWindow::slotCentralityClosenessInfluenceRange(){
    if (!fileLoaded && !networkModified  )  {
        QMessageBox::critical(this, "Error",tr("There are no nodes!\nLoad a network"
                                               " file or create a new network manually. "
                                               "\nThen ask me to compute something!"), "OK",0);
        statusMessage(  QString(tr("Nothing to do..."))  );
        return;
    }

    QString fn = "centrality_closeness_influence_range.dat";
    bool considerWeights=true;

    createProgressBar();

    activeGraph.writeCentralityClosenessInfluenceRange(fn, considerWeights);

    destroyProgressBar();

    statusMessage( QString(tr(" displaying file...")));

    TextEditor *ed = new TextEditor(fn);        //OPEN A TEXT EDITOR WINDOW
    tempFileNameNoPath=fn.split( "/");
    ed->setWindowTitle("Closeness Centrality (influence range) report: " + tempFileNameNoPath.last());
    ed->show();
}




/**
*	Writes Betweeness Centralities into a file, then displays it.
*/
void MainWindow::slotCentralityBetweeness(){
    if (!fileLoaded && !networkModified  )  {
        QMessageBox::critical(this, "Error",tr("There are no nodes!\nLoad a network file or create a new network. \nThen ask me to compute something!"), "OK",0);

        statusMessage(  QString(tr(" Nothing to do..."))  );
        return;
    }
    QString fn = "centrality_betweeness.dat";
    bool considerWeights=true;
    statusMessage(  QString(tr(" Please wait...")));

    createProgressBar();
    activeGraph.writeCentralityBetweeness(fn, considerWeights);
    destroyProgressBar();

    statusMessage( QString(tr(" displaying file...")));

    TextEditor *ed = new TextEditor(fn);        //OPEN A TEXT EDITOR WINDOW
    tempFileNameNoPath=fn.split( "/");
    ed->setWindowTitle("Betweeness Centralities saved as: " + tempFileNameNoPath.last());
    ed->show();
    QApplication::restoreOverrideCursor();
}





/**
*	Writes Degree Prestige indices (In-Degree Centralities) into a file, then displays it.
*/
void MainWindow::slotPrestigeDegree(){
    if (!fileLoaded && !networkModified  )  {
        QMessageBox::critical(this, "Error",tr("Nothing to do!\nLoad a network file or create a new network. \nThen ask me to compute something!"), "OK",0);
        statusMessage(  QString(tr("Nothing to do..."))  );
        return;
    }
    if (activeGraph.isSymmetric()) {
        QMessageBox::critical(this, "Error",tr("Non-directed graph!\nDegree Prestige applies on directed graphs only. Load a digraph, directed network file or create a new network. \nThen ask me to compute it again!"), "OK",0);
        statusMessage(  QString(tr("Nothing to do..."))  );
        return;
    }

    bool considerWeights=false;
    if ( activeGraph.isWeighted()) {
        switch( QMessageBox::information( this, "Degree Prestige (In-Degree)",
                                          tr("Graph edges have weights. \nTake weights into account (Default: No)?"),
                                          tr("Yes"), tr("No"),
                                          0, 1 ) )
        {
        case 0:
            considerWeights=true;
            break;
        case 1:
            considerWeights=false;
            break;
        default: // just for sanity
            considerWeights=false;
            return;
            break;
        }

    }
    QString fn = "degree-prestige.dat";

    createProgressBar();

    activeGraph.writePrestigeDegree(fn, considerWeights);

    destroyProgressBar();

    statusMessage( QString(tr(" displaying file...")));

    TextEditor *ed = new TextEditor(fn);        //OPEN A TEXT EDITOR WINDOW
    tempFileNameNoPath=fn.split( "/");
    ed->setWindowTitle("Degree Prestige (in-degree) saved as: " + tempFileNameNoPath.last());
    ed->show();
}



/**
*	Writes PageRank Prestige indices into a file, then displays it.
*/
void MainWindow::slotPrestigePageRank(){
    if (!fileLoaded && !networkModified  )  {
        QMessageBox::critical(this, "Error",tr("There are no nodes!\nLoad a network file or create a new network. \nThen ask me to compute something!"), "OK",0);

        statusMessage(  QString(tr(" Nothing to do..."))  );
        return;
    }
    QString fn = "prestige_pagerank.dat";
    //   bool considerWeights=false;  //TODO Do we need to compute weigths in PageRank?
    statusMessage(  QString(tr(" Please wait...")));

    createProgressBar();
    activeGraph.writePrestigePageRank(fn);
    destroyProgressBar();

    statusMessage( QString(tr(" displaying file...")));

    TextEditor *ed = new TextEditor(fn);        //OPEN A TEXT EDITOR WINDOW
    tempFileNameNoPath=fn.split( "/");
    ed->setWindowTitle("PageRank Prestige indices saved as: " + tempFileNameNoPath.last());
    ed->show();
    QApplication::restoreOverrideCursor();
}


/**
*	Writes Proximity Prestige indices into a file, then displays them.
*/
void MainWindow::slotPrestigeProximity(){
    if (!fileLoaded && !networkModified  )  {
        QMessageBox::critical(this, "Error",tr("There are no nodes!\nLoad a network file or create a new network. \nThen ask me to compute something!"), "OK",0);

        statusMessage(  QString(tr(" Nothing to do..."))  );
        return;
    }
    QString fn = "centrality_proximity_prestige.dat";
    statusMessage(  QString(tr(" Please wait...")));

    createProgressBar();
    activeGraph.writePrestigeProximity(fn, true);
    destroyProgressBar();

    statusMessage( QString(tr(" displaying file...")));

    TextEditor *ed = new TextEditor(fn);        //OPEN A TEXT EDITOR WINDOW
    tempFileNameNoPath=fn.split( "/");
    ed->setWindowTitle("Proximity Prestige Centralities saved as: " + tempFileNameNoPath.last());
    ed->show();
    QApplication::restoreOverrideCursor();
}




/**
*	Writes Informational Centralities into a file, then displays it.	
    TODO slotCentralityInformation
*/
void MainWindow::slotCentralityInformation(){
    if (!fileLoaded && !networkModified  )  {
        QMessageBox::critical(this, "Error",tr("There are no nodes!\nLoad a network file or create a new network. \nThen ask me to compute something!"), "OK",0);
        statusMessage(  QString(tr(" Nothing to do..."))  );
        return;
    }
    QString fn = "centrality_information.dat";
    statusMessage(  QString(tr(" Please wait...")));

    createProgressBar();
    activeGraph.writeCentralityInformation(fn);
    destroyProgressBar();

    statusMessage( QString(tr(" displaying file...")));

    TextEditor *ed = new TextEditor(fn);
    tempFileNameNoPath=fn.split( "/");
    ed->setWindowTitle("Information Centralities saved as: " + tempFileNameNoPath.last());
    ed->show();
    QApplication::restoreOverrideCursor();

}




/**
*	Writes Stress Centralities into a file, then displays it.
*/
void MainWindow::slotCentralityStress(){
    if (!fileLoaded && !networkModified  )  {
        QMessageBox::critical(this, "Error",tr("There are no nodes!\nLoad a network file or create a new network. \nThen ask me to compute something!"), "OK",0);

        statusMessage(  QString(tr(" Nothing to do! Why don't you try creating something first?"))  );
        return;
    }
    QString fn = "centrality_stress.dat";

    bool considerWeights=true;
    statusMessage(  QString(tr(" Please wait...")));

    createProgressBar();

    activeGraph.writeCentralityStress(fn, considerWeights);

    destroyProgressBar();

    TextEditor *ed = new TextEditor(fn);        //OPEN A TEXT EDITOR WINDOW
    tempFileNameNoPath=fn.split( "/");
    ed->setWindowTitle("Stress Centralities saved as: " + tempFileNameNoPath.last());
    ed->show();
}




/**
*	Writes Power Centralities into a file, then displays it.
*/
void MainWindow::slotCentralityPower(){
    if (!fileLoaded && !networkModified  )  {
        QMessageBox::critical(this, "Error",tr("There are no nodes!\nLoad a network file or create a new network. \nThen ask me to compute something!"), "OK",0);

        statusMessage(  QString(tr(" Nothing to do! Why don't you try creating something first?"))  );
        return;
    }
    QString fn = "centrality_power.dat";

    bool considerWeights=true;
    statusMessage(  QString(tr(" Please wait...")));

    createProgressBar();

    activeGraph.writeCentralityPower(fn, considerWeights);

    destroyProgressBar();

    TextEditor *ed = new TextEditor(fn);        //OPEN A TEXT EDITOR WINDOW
    tempFileNameNoPath=fn.split( "/");
    ed->setWindowTitle("Stress Centralities saved as: " + tempFileNameNoPath.last());
    ed->show();
}


/**
*	Writes Eccentricity Centralities into a file, then displays it.
*/
void MainWindow::slotCentralityEccentricity(){
    if (!fileLoaded && !networkModified  )  {
        QMessageBox::critical(this, "Error",tr("There are no nodes!\nLoad a network file or create a new network. \nThen ask me to compute something!"), "OK",0);

        statusMessage(  QString(tr(" Nothing to do..."))  );
        return;
    }
    QString fn = "centrality_eccentricity.dat";
    bool considerWeights=true;
    statusMessage(  QString(tr(" Please wait...")));

    createProgressBar();
    activeGraph.writeCentralityEccentricity(fn, considerWeights);
    destroyProgressBar();

    TextEditor *ed = new TextEditor(fn);        //OPEN A TEXT EDITOR WINDOW
    tempFileNameNoPath=fn.split( "/");
    ed->setWindowTitle(tr("Eccentricity Centralities saved as: ") + tempFileNameNoPath.last());
    ed->show();
    QApplication::restoreOverrideCursor();

}



void MainWindow::createProgressBar(){

    if (showProgressBarAct->isChecked() || activeGraph.totalEdges() > 2000){
        progressDialog= new QProgressDialog("Please wait, for distance matrix creation....", "Cancel", 0, activeGraph.vertices(), this);
        progressDialog -> setWindowModality(Qt::WindowModal);
        connect( &activeGraph, SIGNAL( updateProgressDialog(int) ), progressDialog, SLOT(setValue(int) ) ) ;
        progressDialog->setMinimumDuration(0);
    }

    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

}


void MainWindow::destroyProgressBar(){
    QApplication::restoreOverrideCursor();

    if (showProgressBarAct->isChecked() || activeGraph.totalEdges() > 1000)
        progressDialog->deleteLater();
}




/**
*	Called from Graph::
*/ 
bool MainWindow::showNumbers(){
    return displayNodeNumbersAct->isChecked();
}





/**
*  Turns on/off displaying the numbers of nodes (outside ones)
*/
void MainWindow::slotDisplayNodeNumbers(bool toggle) {
    if (!fileLoaded && ! networkModified) {
        QMessageBox::critical(this, "Error",tr("There are no nodes! \nLoad a network file or create a new network."), "OK",0);
        statusMessage( tr("Errr...no nodes here. Sorry!") );
        return;
    }
    statusMessage( tr("Toggle Nodes Numbers. Please wait...") );

    if (!toggle) 	{
        graphicsWidget->setAllItemsVisibility(TypeNumber, false);
        statusMessage( tr("Node Numbers are invisible now. Click the same option again to display them.") );
        return;
    }
    else{
        graphicsWidget->setAllItemsVisibility(TypeNumber, true);
        statusMessage( tr("Node Numbers are visible again...") );
    }
}


/**
*	Called by Graph:: and this->initNet()
*/
bool MainWindow::showLabels(){
    return displayNodeLabelsAct->isChecked();
}



/**
*	Called by Graph:: and this->initNet()
*/
bool MainWindow::showNumbersInsideNodes(){
    return displayNumbersInsideNodesAct->isChecked();
}



/**
*  Turns on/off displaying the nodenumbers inside the nodes.
*/
void MainWindow::slotDisplayNumbersInsideNodes(bool toggle){
    statusMessage( tr("Toggle Numbers inside nodes. Please wait...") );

    if ( showNumbers() ) 	{
        // ?
    }
    else{
        displayNodeNumbersAct->setChecked(true);
    }

    activeGraph.setShowNumbersInsideNodes(toggle);
    graphicsWidget -> setNumbersInsideNodes(toggle);

    if (toggle){

        statusMessage( tr("Numbers inside nodes...") );
    }
    else {

        statusMessage( tr("Numbers outside nodes...") );
    }
}

/**
*  Turns on/off displaying labels
*/
void MainWindow::slotDisplayNodeLabels(bool toggle){
    if (!fileLoaded && ! networkModified) {
        QMessageBox::critical(this, "Error",tr("There are no nodes! \nLoad a network file or create a new network first. "), "OK",0);
        statusMessage( tr("No nodes found. Sorry...") );
        return;
    }
    statusMessage( tr("Toggle Nodes Labels. Please wait...") );

    if (!toggle) 	{
        graphicsWidget->setAllItemsVisibility(TypeLabel, false);
        statusMessage( tr("Node Labels are invisible now. Click the same option again to display them.") );
        return;
    }
    else{
        graphicsWidget->setAllItemsVisibility(TypeLabel, true);
        statusMessage( tr("Node Labels are visible again...") );
    }
    activeGraph.setShowLabels(toggle);
}




/**
*   Changes the size of all nodes
*/
void MainWindow::slotChangeAllNodesSize() {
    bool ok=false;

    int newSize = QInputDialog::getInt(this, "Change node size", tr("Select new size for all nodes: (1-16)"),initNodeSize, 1, 16, 1, &ok );
    if (!ok) {
        statusMessage( "Change node size operation cancelled." );
        return;
    }

    qDebug ("MW: slotChangeAllNodesSize:");
    changeAllNodesSize(newSize);
    graphChanged();
    statusBar()->showMessage (QString(tr("Ready")), statusBarDuration) ;
    return;
}


/**
*   Changes the size of nodes. 
*/
void MainWindow::changeAllNodesSize(int size) {
    qDebug ("MW: changeAllNodesSize:");
    if (size == 0 ) {
        if (activeNodes() < 200) {
            return;
        }
        else if (activeNodes() >= 200 && activeNodes() < 500){
            size = 4;
        }
        else if (activeNodes() >= 500 && activeNodes() < 1000) {
            size = 3;
        }
        else if (activeNodes() >= 1000) {
            size = 2;
        }
    }
    initNodeSize = size;
    activeGraph.setInitVertexSize(initNodeSize);
    qDebug () << "MW: changeAllNodesSize: changing to " <<  size;
    QList<QGraphicsItem *> list=scene->items();
    for (QList<QGraphicsItem *>::iterator it=list.begin(); it!=list.end(); it++)
        if ( (*it) -> type() == TypeNode ){
            Node *jim = (Node*) (*it);
            (*jim).setSize(size);
        }
}


/**
*  Changes the shape of all nodes. 
*/
void MainWindow::slotChangeAllNodesShape() {
    bool ok=false;
    QStringList lst;
    lst << "box"<< "circle"<< "diamond"<< "ellipse"<< "triangle";
    QString newShape = QInputDialog::getItem(this, "Node shapes", "Select a shape for all nodes: ", lst, 1, true, &ok);
    if ( ok ) {
        //user selected an item and pressed OK
        QList<QGraphicsItem *> list=scene->items();
        for (QList<QGraphicsItem *>::iterator it=list.begin(); it!=list.end(); it++)
            if ( (*it) -> type() == TypeNode ){
                Node *jim = (Node*) (*it);
                (*jim).setShape(newShape);
                activeGraph.setVertexShape ((*jim).nodeNumber(), newShape);
            }
        graphChanged();
        activeGraph.setInitVertexShape(newShape);
        statusBar()->showMessage (QString(tr("All shapes have been changed. Ready")), statusBarDuration) ;
    } else {
        //user pressed Cancel
        statusBar()->showMessage (QString(tr("Change node shapes aborted...")), statusBarDuration) ;
    }

}



/**
*  Change size of all nodes' numbers (outside ones)
*/
void MainWindow::slotChangeNumbersSize() {
    bool ok=false;
    int newSize;
    newSize = QInputDialog::getInt(this, "Change text size", tr("Change all nodenumbers size to: (1-16)"),initNumberSize, 1, 16, 1, &ok );
    if (!ok) {
        statusMessage( tr("Change font size: Aborted.") );
        return;
    }

    QList<QGraphicsItem *> list=scene->items();
    for (QList<QGraphicsItem *>::iterator it2=list.begin();it2!=list.end(); it2++)

        if ( (*it2)->type()==TypeNumber) {
            NodeNumber * number= (NodeNumber*) (*it2);
            qDebug ("MW: slotChangeNumbersSize Found");
            number->setFont( QFont (number->font().family(), newSize, QFont::Light, false) );
        }

    activeGraph.setInitVertexNumberSize(newSize);
    statusMessage( tr("Changed numbers size. Ready.") );
}


/**
*  Changes size of all nodes' labels
*/
void MainWindow::slotChangeLabelsSize() {
    bool ok=false;
    int newSize;
    newSize = QInputDialog::getInt(this, "Change text size", tr("Change all node labels size to: (1-16)"),initNumberSize, 1, 16, 1, &ok );
    if (!ok) {
        statusMessage( tr("Change font size: Aborted.") );
        return;
    }
    QList<QGraphicsItem *> list=scene->items();
    for (QList<QGraphicsItem *>::iterator it2=list.begin();it2!=list.end(); it2++)

        if ( (*it2)->type()==TypeLabel) {
            NodeLabel *label= (NodeLabel*) (*it2);
            qDebug ("MW: slotChangeLabelsSize Found");
            label->setFont( QFont (label->font().family(), newSize, QFont::Light, false) );
            activeGraph.setVertexLabelSize ( (label->node())->nodeNumber(), newSize);
        }
    activeGraph.setInitVertexLabelSize(newSize);
    statusMessage( tr("Changed labels size. Ready.") );
}





/**
    Turns on/off drawing links as thick as their weights.
    TODO
*/
void MainWindow::slotDrawLinksThickAsWeights() {

}



/**
*  Turns on/off displaying link weight numbers
*/
void MainWindow::slotDisplayLinksWeightNumbers(bool toggle) {
    if (!fileLoaded && ! networkModified) {
        QMessageBox::critical(this, "Error",tr("There are no links! \nLoad a network file or create a new network first."), "OK",0);
        statusMessage( tr("No nodes or edges found. Sorry...") );
        return;
    }
    qDebug() << "MW: slotDisplayLinksWeightNumbers - Toggling Edges Weights. Please wait...";

    statusMessage( tr("Toggle Edges Weights. Please wait...") );

    if (!toggle) 	{
        graphicsWidget->setAllItemsVisibility(TypeEdgeWeight, false);
        statusMessage( tr("Edge weights are invisible now. Click the same option again to display them.") );
        return;
    }
    else{
        graphicsWidget->setAllItemsVisibility(TypeEdgeWeight, true);
        statusMessage( tr("Edge weights are visible again...") );
    }
    activeGraph.setShowLabels(toggle);
}



/**
*  Turns on/off displaying links
*/
void MainWindow::slotDisplayLinks(bool toggle){
    if (!fileLoaded && ! networkModified) {
        QMessageBox::critical(this, "Error",tr("There are no nodes nor links! \nLoad a network file or create a new network first!"), "OK",0);

        statusMessage( tr("No links found...") );
        return;
    }
    statusMessage( tr("Toggle Edges Arrows. Please wait...") );

    if (!toggle) 	{
        graphicsWidget->setAllItemsVisibility(TypeEdge, false);
        statusMessage( tr("Links are invisible now. Click again the same menu to display them.") );
        return;
    }
    else{
        graphicsWidget->setAllItemsVisibility(TypeEdge, true);
        statusMessage( tr("Links visible again...") );
    }

}



/**
*  Turns on/off the arrows of links
*/
void MainWindow::slotDisplayLinksArrows(bool toggle){
    if (!fileLoaded && ! networkModified) {
        QMessageBox::critical(this, "Error",tr("There are no links! \nLoad a network file or create a new network first!"), "OK",0);

        statusMessage( tr("No links found...") );
        return;
    }
    statusMessage( tr("Toggle Edges Arrows. Please wait...") );

    if (!toggle) 	{
        QList<QGraphicsItem *> list = scene->items();
        for (QList<QGraphicsItem *>::iterator item=list.begin();item!=list.end(); item++) {
            if ( (*item)->type() ==TypeEdge){
                Edge *edge = (Edge*) (*item);
                edge->showArrows(false);
            }
        }
        return;
    }
    else{
        QList<QGraphicsItem *> list = scene->items();
        for (QList<QGraphicsItem *>::iterator item=list.begin();item!=list.end(); item++)
            if ( (*item)->type() ==TypeEdge){
                Edge *edge = (Edge*) (*item);
                edge->showArrows(true);
            }
    }
    statusMessage( tr("Ready."));
}



/**
*  FIXME links Bezier 
*/
void MainWindow::slotDrawLinksBezier(bool toggle){
    if (!fileLoaded && ! networkModified) {
        QMessageBox::critical(this, "Error",tr("There are no links! \nLoad a network file or create a new network!"), "OK",0);

        statusMessage( tr("There are NO links here!") );
        return;
    }
    statusMessage( tr("Toggle links bezier. Please wait...") );
    // //	graphicsWidget->setBezier(toggle);
    if (!toggle) 	{
        // 		QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
        // 		QList<QGraphicsItem *> list = scene->items();
        // 		for (QList<QGraphicsItem *>::iterator item=list.begin();item!=list.end(); item++) {
        // 			if ( (*item)->type() ==TypeEdge ){
        // 				Edge *edge = (Edge*) (*item);
        // //				edge->toggleBezier(false);
        // 				(*item)->hide();(*item)->show();
        // 			}
        //
        // 		}
        // 		QApplication::restoreOverrideCursor();
        // 		return;
    }
    else{
        // 		QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
        // 		QList<QGraphicsItem *> list = scene->items();
        // 		for (QList<QGraphicsItem *>::iterator item=list.begin();item!=list.end(); item++){
        // 			if ( (*item)->type() ==TypeEdge ){
        // 				Edge *edge = (Edge*) (*item);
        // //				edge->toggleBezier(true);
        // 				(*item)->hide();(*item)->show();
        // 			}
        // 		}
        // 		QApplication::restoreOverrideCursor();
    }

}



/**
*  Changes the background color of the scene
*/
void MainWindow::slotBackgroundColor () {
    qDebug("MW: slotBackgroundColor ");
    QColor backgrColor = QColorDialog::getColor( initBackgroundColor, this );
    graphicsWidget ->setBackgroundBrush(QBrush(backgrColor));
    statusMessage( tr("Ready. ") );
}


/**
*  Changes the color of all nodes
*/
void MainWindow::slotAllNodesColor(){
    bool ok=false;
    initNodeColor = QInputDialog::getItem(this, "Nodes' colors", "Select a new color:", colorList, 1, true, &ok);
    if ( ok ) {
        QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
        qDebug ("MW: Will change color");
        QList<QGraphicsItem *> list= scene->items();
        for (QList<QGraphicsItem *>::iterator it=list.begin(); it!=list.end(); it++)
            if ( (*it)->type() == TypeNode ) 	{
                Node *jim = (Node *) (*it);
                jim->setColor(initNodeColor);
                qDebug ("MW: Changed color");
                activeGraph.setVertexColor (jim->nodeNumber(), initNodeColor);
                graphChanged();
            }
        activeGraph.setInitVertexColor(initNodeColor);
        QApplication::restoreOverrideCursor();
        statusMessage( tr("Ready. ")  );
    }
    else {
        // user pressed Cancel
        statusMessage( tr("Change node color aborted. ")  );
    }

}



/**
*  Changes the color of all links
*/
void MainWindow::slotAllLinksColor(){
    bool ok=false;
    initLinkColor = QInputDialog::getItem(this, "Links' colors", "Select a new color:", colorList, 1, true, &ok);
    if ( ok ) {
        QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
        qDebug ("MW: Will change color");
        QList<QGraphicsItem *> list= scene->items();
        for (QList<QGraphicsItem *>::iterator it=list.begin(); it!=list.end(); it++)
            if ( (*it)->type() == TypeEdge ) 	{
                Edge *link = (Edge *) (*it);
                link->setColor(initLinkColor );
                qDebug ("MW: Changed color");
                activeGraph.setEdgeColor (link->sourceNodeNumber(), link->targetNodeNumber(), initLinkColor);
                graphChanged();
            }
        activeGraph.setInitEdgeColor(initLinkColor);
        QApplication::restoreOverrideCursor();
        statusMessage( tr("Ready. ")  );

    }
    else {
        // user pressed Cancel
        statusMessage( tr("Change link color aborted. ")  );
    }
}




/**
*  Changes the color of nodes' numbers
*/
void MainWindow::slotAllNumbersColor(){
    // = QInputDialog::getItem(this, "Links' colors", "Select a new color:", colorList, 1, true, &ok);
    QColor textColor = QColorDialog::getColor( Qt::black, this );
    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
    qDebug ("MW: Will change color");
    QList<QGraphicsItem *> list= scene->items();
    for (QList<QGraphicsItem *>::iterator it=list.begin(); it!=list.end(); it++) {
        if ( (*it)->type() == TypeNumber) 		{
            NodeNumber *jimNumber = (NodeNumber *) (*it);
            jimNumber->update();
            jimNumber->setDefaultTextColor(textColor);
        }
    }
    activeGraph.setInitVertexNumberColor( textColor.name() );
    QApplication::restoreOverrideCursor();
    statusMessage( tr("Numbers' colors changed. Ready. ")  );
}



/**
*  Changes the color of nodes labels
*/
void MainWindow::slotAllLabelsColor(){
    QColor textColor = QColorDialog::getColor( Qt::black, this );
    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
    qDebug ("MW: Will change label color");
    QList<QGraphicsItem *> list= scene->items();
    for (QList<QGraphicsItem *>::iterator it=list.begin(); it!=list.end(); it++)
        if ( (*it)->type() == TypeNode ) 	{
            Node *jim = (Node *) (*it);
            jim->label()->update();
            jim->label()->setDefaultTextColor(textColor);
            qDebug ("MW: Changed color");
            activeGraph.setVertexLabelColor (jim->nodeNumber(), textColor.name());
        }
    activeGraph.setInitVertexLabelColor(textColor.name());
    QApplication::restoreOverrideCursor();
    statusMessage( tr("Label colors changed. Ready. ")  );
}



/**
*  turns antialiasing on or off
*/
void MainWindow::slotAntialiasing(bool toggle) {
    statusMessage( tr("Toggle anti-aliasing. This will take some time if the network is large (>500)...") );
    //Inform graphicsWidget about the change
    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
    graphicsWidget->setRenderHint(QPainter::Antialiasing, toggle);
    graphicsWidget->setRenderHint(QPainter::TextAntialiasing, toggle);
    graphicsWidget->setRenderHint(QPainter::SmoothPixmapTransform, toggle);
    QApplication::restoreOverrideCursor();
    if (!toggle)
        statusMessage( tr("Anti-aliasing off.") );
    else
        statusMessage( tr("Anti-aliasing on.") );

}

/**
*  turn progressbar on or off
*/
void MainWindow::slotShowProgressBar(bool toggle) {
    statusMessage( tr("Toggle progressbar..."));
    if (!toggle)  {
        statusMessage( tr("Progress bars off.") );
    }
    else   {
        statusMessage( tr("Progress bars on.") );
    }
}



/**
*  Turns debugging messages on or off
*/
void MainWindow::slotPrintDebug(bool toggle){
    if (!toggle)   {
        printDebug=false;
        statusMessage( tr("Debug messages off.") );
    }
    else  {
        printDebug=true;
        statusMessage( tr("Debug messages on.") );
    }
}




/**
*  Turns Toolbar on or off
*/
void MainWindow::slotViewToolBar(bool toggle) {
    statusMessage( tr("Toggle toolbar..."));
    if (toggle== false)   {
        toolBar->hide();
        statusMessage( tr("Toolbar off.") );
    }
    else  {
        toolBar->show();
        statusMessage( tr("Toolbar on.") );
    }
}



/**
*  Turns Statusbar on or off
*/
void MainWindow::slotViewStatusBar(bool toggle) {
    statusMessage( tr("Toggle statusbar..."));

    if (toggle == false)   {
        statusBar()->hide();
        statusMessage( tr("Status bar off.") );
    }
    else   {
        statusBar()->show();
        statusMessage( tr("Status bar on.") );
    }

}


/*
 * Enables/disables displaying a user-defined custom image in the background
 */
void MainWindow::slotBackgroundImage(bool toggle) {
    statusMessage( tr("Toggle BackgroundImage..."));
    QString m_fileName ;
    if (toggle == false)   {
        statusMessage( tr("BackgroundImage off.") );
        graphicsWidget->setBackgroundBrush(QBrush(initBackgroundColor));
    }
    else   {
        m_fileName = QFileDialog::getOpenFileName( this, tr("Select one image"), "",
                                                   tr("All (*);;PNG (*.png);;JPG (*.jpg)")
                                                   );
        graphicsWidget->setBackgroundBrush(QImage(m_fileName));
        graphicsWidget->setCacheMode(QGraphicsView::CacheBackground);

        statusMessage( tr("BackgroundImage on.") );
    }


}

/**
*  Displays a random tip
*/
void MainWindow::slotTips() {
    int randomTip=rand()%tipsCounter; //Pick a tip.
    QMessageBox::about( this, tr("Tip Of The Day"), tips[randomTip]);
}



/**
    Creates our tips.
*/
void MainWindow::createTips(){
    tips+=tr("You can add a new node by double-clicking on the scene.");
    tips+=tr("You can add a new node by clicking on Add button.");
    tips+=tr("You can remove a node by clicking on Remove button.");
    tips+=tr("You can rotate the network by selecting a new angle on the dock.");
    tips+=tr("You can add a new link between two nodes, by middle-clicking (or pressing both mouse buttons simultanesously) on the first and then on the second node.");
    tips+=tr("You can remove a node by right-clicking on it and selecting Remove.");
    tips+=tr("You can change background color (from the menu Edit > Colors).");
    tips+=tr("Nodes can have the colors of your choice. Just right-click on a node and then select > Options > Change Color. You can select every color supported by the X.org pallette.");
    tips+=tr("The tabs on the left dock show information about the network (nodes, edges, density, etc) as well as information about any node you clicked on (inDegrees, outDegrees, clustering).");
    tips+=tr("You can move a node easily by dragging it with your mouse.");
    tips+=tr("SocNetV can save the positions of the nodes in a network, if you save it in Pajek/GraphML format.");
    tips+=tr("You can apply layout algorithms on the network from the menu Layout or by clicking on the Dock > Layout tab checkboxes");
    tips+=tr("You can change the label of node by right-clicking on it, and selecting Options > Change Label.");
    tips+=tr("All basic operations of SocNetV are available from the dock on the left, or by right-clicking on a node or a link.");
    tips+=tr("Node information is displayed on the Status bar, when you left-click on it.");
    tips+=tr("Link information is displayed on the Status bar, when you left-click on it.");

    tipsCounter = 16;
}




/**
    Loads the HTML Help file and displays it via HTMLViewer.
*/
void MainWindow::slotHelp(){

    QString helpPath;
    bool manualFound = false;
    QDir d( QCoreApplication::applicationDirPath() );
    qDebug()<< QCoreApplication::applicationDirPath().toLatin1();

    if ( d.exists("manual.html") ) {
        helpPath=d.filePath("manual.html");
    }
    else {
        if (d.dirName()=="bin") {
            d.cdUp();
        }
        if (d.cd("./manual") ) {
            if ( d.exists("manual.html") ) {
                helpPath=d.filePath("manual.html");
                manualFound = true;
            }
            else 	{
                qDebug()<< "help file does not exist here.";
                manualFound = false;
            }
        }
        if (d.cd("../../../manual") ) {         // for Mac
            if ( d.exists("manual.html") ) {
                helpPath=d.filePath("manual.html");
                manualFound = true;
            }
            else 	{
                qDebug()<< "help file does not exist here.";
                manualFound = false;
            }
        }

        if (!manualFound && d.cd("../trunk/manual") ) {
            if ( d.exists("manual.html") ) {
                helpPath=d.filePath("manual.html");
                manualFound = true;
            }
            else 	{
                qDebug()<< "help file does not exist here.";
                manualFound = false;
            }
        }
        if ( !manualFound && d.cd("/usr/local/share/doc/socnetv/") ) {			//for compile installation
            if (d.exists("manual/")) d.cd("manual/");
            if ( d.exists("manual.html") ) {
                helpPath=d.filePath("manual.html");
                qDebug()<< "path" << helpPath.toLatin1();
                manualFound = true;
            }
            else {
                qDebug()<< "help file does not exist.";
                manualFound = false;
            }
        }
        if (!manualFound && d.cd("/usr/share/doc/socnetv/") ) {     //for Debian Ubuntu
            if (d.exists("manual/")) d.cd("manual/");
            if ( d.exists("manual.html") ) {
                helpPath=d.filePath("manual.html");
                manualFound = true;
            }
            else {
                qDebug("help file does not exist in /usr/share/doc/socnetv/.");
                manualFound = false;
            }
        }
        if ( !manualFound && d.cd("/usr/share/doc/packages/socnetv/") ) {  //for opensuse file hierarchy
            if (d.exists("manual/"))
                d.cd("manual/");
            if ( d.exists("manual.html") ) {
                helpPath=d.filePath("manual.html");
                manualFound = true;
            }
            else {
                qDebug("help file does not exist.");
            }
        }
        QString fedoraPath = "/usr/share/doc/socnetv-" + VERSION;
        if ( !manualFound && d.cd(fedoraPath) ) {  //for Fedora file hierarchy
            if (d.exists("manual/"))
                d.cd("manual/");
            if ( d.exists("manual.html") ) {
                helpPath=d.filePath("manual.html");
                manualFound = true;
            }
            else {
                qDebug("help file does not exist.");
            }
        }

    }
    qDebug () << "help path is: " << helpPath.toLatin1();
    HTMLViewer *helpViewer = new HTMLViewer (helpPath, this);
    helpViewer -> setWindowTitle ( "SocNetV "+ VERSION + tr(" Manual"));
    helpViewer->show();

}



/**
    Displays the following message!!
*/
void MainWindow::slotHelpAbout(){
    int randomCookie=rand()%fortuneCookiesCounter;//createFortuneCookies();
QString BUILD="Tue Aug  5 21:58:23 EEST 2014";
    QMessageBox::about( this, "About SocNetV",
                        "<b>Soc</b>ial <b>Net</b>work <b>V</b>isualizer (SocNetV)"
                        "<p><b>Version</b>: " + VERSION + "</p>"
                        "<p><b>Build</b>: "  + BUILD + " </p>"

                        "<p>(C) 2005-2014 by Dimitris V. Kalamaras"
                        "<br> dimitris.kalamaras@gmail.com"

                        "<p><b>Fortune cookie: </b><br> \""  + fortuneCookie[randomCookie]  +"\""

                        "<p><b>License:</b><br>"

                        "This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version.</p>"

                        "<p>This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.</p>"

                        "<p>You should have received a copy of the GNU General Public License along with this program; If not, see http://www.gnu.org/licenses/</p>");
}



/**
    Creates the fortune cookies displayed on the above message.
*/
void MainWindow::createFortuneCookies(){
    fortuneCookie+="sic itur ad astra / sic transit gloria mundi ? <br /> --Unknown";
    fortuneCookie+="losers of yesterday, the winners of tomorrow... <br /> --B.Brecht";
    fortuneCookie+="Patriotism is the virtue of the wicked... <br /> --O. Wilde";

    fortuneCookie+="No tengo nunca mas, no tengo siempre. En la arena <br />"
            "la victoria dejo sus piers perdidos.<br />"
            "Soy un pobre hombre dispuesto a amar a sus semejantes.<br />"
            "No se quien eres. Te amo. No doy, no vendo espinas. <br /> --Pablo Neruda"  ;
    fortuneCookie+="I will never apologize for the United States of America. I don't care what it has done. I don't care what the facts are. <br> --Vice President George H.W. Bush, after the Iranian airliner flight IR655 (an Airbus A300) was shot down by a U.S. missile cruiser (USS Vincennes), killing all 290 civilian passengers...";
    fortuneCookie+="Man must not check reason by tradition, but contrawise, must check tradition by reason.<br> --Leo Tolstoy";
    fortuneCookie+="Only after the last tree has been cut down, <br>only after the last river has been poisoned,<br> only after the last fish has been caught,<br>only then will you realize that money cannot be eaten. <br> --The Cree People";
    fortuneCookie+="Stat rosa pristina nomine, nomina nuda tenemus <br > --Unknown";
    fortuneCookie+="Jupiter and Saturn, Oberon, Miranda <br />"
            "And Titania, Neptune, Titan. <br />"
            "Stars can frighten. <br /> Syd Barrett";

    fortuneCookiesCounter=9;
    //   return fortuneCookie.count();
}




/**
    Displays a short message about the Qt Toolbox.
*/
void MainWindow::slotAboutQt(){
    QMessageBox::aboutQt(this, "About Qt - SocNetV");
}



