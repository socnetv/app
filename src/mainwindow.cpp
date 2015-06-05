/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 1.8
 Written in Qt

-                           mainwindow.cpp  -  description
                             -------------------
    copyright            : (C) 2005-2015 by Dimitris B. Kalamaras
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
#include <QKeySequence>


#include "mainwindow.h"
#include "graphicswidget.h"
#include "node.h"
#include "edge.h"
#include "nodenumber.h"
#include "nodelabel.h"
#include "edgeweight.h"
#include "texteditor.h"
#include "filteredgesbyweightdialog.h"
#include "guide.h"
#include "vertex.h"
#include "previewform.h"
#include "randerdosrenyidialog.h"
#include "randsmallworlddialog.h"
#include "randscalefreedialog.h"




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
             this, SLOT ( edgeInfoStatusBar(Edge*) )  );

    connect( graphicsWidget, SIGNAL( windowResized(int, int)),
             this, SLOT( windowInfoStatusBar(int,int)) 	);

    connect( graphicsWidget, SIGNAL( windowResized(int, int)),
             &activeGraph, SLOT( setCanvasDimensions(int,int)) 	);

    connect( graphicsWidget, SIGNAL( userDoubleClicked(int, QPointF) ),
             this, SLOT( addNodeWithMouse(int,QPointF) ) ) ;

    connect( graphicsWidget, SIGNAL( userMiddleClicked(int, int, float) ),
             this, SLOT( addEdge(int, int, float) ) 	);


    connect( graphicsWidget, SIGNAL( openNodeMenu() ),
             this, SLOT( openNodeContextMenu() ) ) ;

    connect( graphicsWidget, SIGNAL( openEdgeMenu() ),
             this, SLOT( openEdgeContextMenu() ) ) ;

    connect (graphicsWidget, &GraphicsWidget::openContextMenu,
             this, &MainWindow::openContextMenu);

    connect( graphicsWidget, SIGNAL(updateNodeCoords(int, int, int)),
             this, SLOT( updateNodeCoords(int, int, int) ) );

    connect( graphicsWidget, SIGNAL(zoomChanged(int)),
             zoomCombo, SLOT( setCurrentIndex(int)) );

    connect( &activeGraph, SIGNAL( addGuideCircle(int, int, int) ),
             graphicsWidget, SLOT(  addGuideCircle(int, int, int) ) ) ;

    connect( &activeGraph, SIGNAL( addGuideHLine(int) ),
             graphicsWidget, SLOT(  addGuideHLine(int) ) ) ;

    connect( &activeGraph, SIGNAL( moveNode(int, qreal, qreal) ),
             graphicsWidget, SLOT( moveNode(int, qreal, qreal) ) ) ;


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


    connect( &activeGraph, SIGNAL( setEdgeColor(long int,long int,QString)),
             graphicsWidget, SLOT( setEdgeColor(long int,long int,QString) ) );


    connect( &activeGraph, SIGNAL( statusMessage (QString) ),
             this, SLOT( statusMessage (QString) ) ) ;

    connect( &activeGraph, SIGNAL( describeDataset (QString) ),
             this, SLOT( showMessageToUser (QString) ) ) ;

    connect( &activeGraph, SIGNAL( eraseNode(long int) ),
             graphicsWidget, SLOT(  eraseNode(long int) ) );

    connect( &activeGraph, &Graph::signalNodeSizesByInDegree,
             this, &MainWindow::slotLayoutNodeSizesByInDegree );


    //connect some signals/slots with MW widgets
    connect( addNodeBt,SIGNAL(clicked()), this, SLOT( addNode() ) );

    connect( addEdgeBt,SIGNAL(clicked()), this, SLOT( slotAddEdge() ) );

    connect( removeNodeBt,SIGNAL(clicked()), this, SLOT( slotRemoveNode() ) );

    connect( removeEdgeBt,SIGNAL(clicked()), this, SLOT( slotRemoveEdge() ) );

    connect( zoomCombo, SIGNAL(currentIndexChanged(const int &)),
             graphicsWidget, SLOT( changeZoom(const int &))  );

    connect( zoomOutAct, SIGNAL(triggered()), graphicsWidget, SLOT( zoomOut() ) );
    connect( zoomInAct, SIGNAL(triggered()), graphicsWidget, SLOT( zoomIn() ) );

    connect( rotateSpinBox, SIGNAL(valueChanged(int)), graphicsWidget, SLOT( rot(int) ) );

    connect( nextRelationAct, SIGNAL(triggered()), this, SLOT( nextRelation() ) );
    connect( prevRelationAct, SIGNAL(triggered()), this, SLOT( prevRelation() ) );
    connect( addRelationAct, SIGNAL(triggered()), this, SLOT( addRelation() ) );

    connect( changeRelationCombo , SIGNAL( currentIndexChanged(int) ) ,
             &activeGraph, SLOT( changeRelation(int) ) );

    connect( this , SIGNAL(addRelationToGraph(QString)),
             &activeGraph, SLOT( addRelationFromUser(QString) ) );

    connect ( &activeGraph, SIGNAL(addRelationToMW(QString)),
              this, SLOT(addRelation(QString)));

    connect( &activeGraph, SIGNAL(relationChanged(int)),
             graphicsWidget, SLOT( changeRelation(int))  ) ;


    connect( &m_filterEdgesByWeightDialog, SIGNAL( userChoices( float, bool) ),
             &activeGraph, SLOT( filterEdgesByWeight (float, bool) ) );


    connect( &m_WebCrawlerDialog, &WebCrawlerDialog::userChoices,
             this, &MainWindow::slotWebCrawl );

    connect( &m_datasetSelectDialog, SIGNAL( userChoices( QString) ),
             this, SLOT( slotRecreateDataSet(QString) ) );

    connect( &activeGraph, SIGNAL( setEdgeVisibility (int, int, int, bool) ),
             graphicsWidget, SLOT(  setEdgeVisibility (int, int, int, bool) ) );

    connect( &activeGraph, SIGNAL( setVertexVisibility(long int, bool)  ),
             graphicsWidget, SLOT(  setNodeVisibility (long int ,  bool) ) );

    connect( &activeGraph, SIGNAL( setNodeSize(long int, int)  ),
             graphicsWidget, SLOT(  setNodeSize (long int , int) ) );

    connect( &activeGraph, SIGNAL( setNodeShape(const long int, const QString)  ),
             graphicsWidget, SLOT(  setNodeShape (const long int , const QString) ) );

    connect( &activeGraph, SIGNAL( setNodeColor(long int,QString))  ,
             graphicsWidget, SLOT(  setNodeColor(long int, QString) ) );

    connect( &activeGraph, &Graph::setNodeLabel ,
             graphicsWidget, &GraphicsWidget::setNodeLabel );

    connect( clearGuidesAct, SIGNAL(triggered()),
             graphicsWidget, SLOT(clearGuides()));

    connect(toolBoxAnalysisGeodesicsSelect, SIGNAL (currentIndexChanged(int) ),
            this, SLOT(toolBoxAnalysisGeodesicsSelectChanged(int) ) );

    connect(toolBoxAnalysisConnectivitySelect, SIGNAL (currentIndexChanged(int) ),
            this, SLOT(toolBoxAnalysisConnectivitySelectChanged(int) ) );

    connect(toolBoxAnalysisClusterabilitySelect, SIGNAL (currentIndexChanged(int) ),
            this, SLOT(toolBoxAnalysisClusterabilitySelectChanged(int) ) );

    connect(toolBoxAnalysisProminenceSelect, SIGNAL (currentIndexChanged(int) ),
            this, SLOT(toolBoxAnalysisProminenceSelectChanged(int) ) );


    connect(toolBoxLayoutByIndexButton, SIGNAL (clicked() ),
            this, SLOT(toolBoxLayoutByIndexButtonPressed() ) );

    connect( layoutGuidesBx, SIGNAL(stateChanged(int)),
             this, SLOT(slotLayoutGuides(int)));



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
    qDebug()<<"   initialise default network parameters";
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
    dataDir= QDir::homePath() +QDir::separator() + "socnetv-data" + QDir::separator() ;
    lastUsedDirPath = "socnetv-initial-none";
    if (firstTime) {
        createFortuneCookies();
        createTips();
        QDir ourDir(dataDir);
        if ( !ourDir.exists() ) {
            ourDir.mkdir(dataDir);
            QMessageBox::information(this, "SocNetV Data Directory",
                                 tr("SocNetV saves reports and files in the "
                                    "directory %1")
                                 .arg (ourDir.absolutePath())
                                 , QMessageBox::Ok, 0);

        }

    }

    qDebug() << "MW::MainWindow() call findCodecs" ;
    findCodecs();

    qDebug() << "MW::MainWindow() create PreviewForm object and set codecs" ;
    previewForm = new PreviewForm(this);
    previewForm->setCodecList(codecs);

    connect (previewForm, &PreviewForm::userCodec, this, &MainWindow::userCodec );

    qDebug() << "MW::MainWindow() Try load *graphml* file on exec time ";
    if (!m_fileName.isEmpty())
    {
        fileName=m_fileName;
        fileNameNoPath=fileName.split ("/");
        previewNetworkFile( fileName, 0 );
    }

    graphicsWidget->setFocus();

    statusMessage( tr("Welcome to Social Network Visualizer, Version ")+VERSION);

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

    importDot = new QAction( QIcon(":/images/open.png"), tr("GraphViz (.dot)"), this);
    importDot->setStatusTip(tr("Import an dot file"));
    importDot->setWhatsThis(tr("Import GraphViz \n\n  Imports a network from an GraphViz formatted file"));
    connect(importDot, SIGNAL(triggered()), this, SLOT(slotImportDot()));


    importDL = new QAction( QIcon(":/images/open.png"), tr("UCINET (.dl)..."), this);
    importDL->setStatusTip(tr("Import network to a DL-formatted file (UCINET)"));
    importDL->setWhatsThis(tr("Import UCINET\n\nImports a network from a DL-formatted file"));
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


    openTextEditorAct = new QAction(QIcon(":/images/texteditor.png"),
                                    tr("Open Text Editor"),this);
    openTextEditorAct ->setShortcut(tr("Shift+F5"));
    openTextEditorAct->setStatusTip(tr("Opens the SocNetV text editor."
                                       "You can copy/paste network data, save and then import them..."));
    openTextEditorAct->setWhatsThis(tr("Open Text Editor\n\nOpens the SocNetV text editor where you can copy paste network data, of any supported format, and save to a file. Then you can import that file to SocNetV..."));
    connect(openTextEditorAct, SIGNAL(triggered()), this, SLOT(slotOpenTextEditor()));


    viewNetworkFileAct = new QAction(QIcon(":/images/networkfile.png"), tr("View Loaded File"),this);
    viewNetworkFileAct ->setShortcut(tr("F5"));
    viewNetworkFileAct->setStatusTip(tr("Displays the loaded network file"));
    viewNetworkFileAct->setWhatsThis(tr("View Loaded File\n\nDisplays the file of the loaded network"));
    connect(viewNetworkFileAct, SIGNAL(triggered()), this, SLOT(slotViewNetworkFile()));

    viewSociomatrixAct = new QAction(QIcon(":/images/sm.png"), tr("View Adjacency Matrix"),  this);
    viewSociomatrixAct ->setShortcut(tr("F6"));
    viewSociomatrixAct->setStatusTip(tr("Displays the adjacency matrix of the active network. See manual or online help for more..."));
    viewSociomatrixAct->setWhatsThis(tr("View Adjacency Matrix\n\nDisplays the adjacency matrix of the active network. \n\n The adjacency matrix of a network is a matrix where each element a(i,j) is equal to the weight of the Edge from node i to node j. If the nodes are not connected, then a(i,j)=0. "));
    connect(viewSociomatrixAct, SIGNAL(triggered()), this, SLOT(slotViewAdjacencyMatrix()));


    recreateDataSetAct = new QAction(QIcon(":/images/sm.png"), tr("Create Known Data Sets"),  this);
    recreateDataSetAct ->setShortcut(tr("F7"));
    recreateDataSetAct->setStatusTip(tr("Recreates a variety of known data sets."));
    recreateDataSetAct->setWhatsThis(tr("Known Data Sets\n\nRecreates some of the most widely used data sets in network analysis studies"));
    connect(recreateDataSetAct, SIGNAL(triggered()), this, SLOT(slotShowDataSetSelectDialog()));



    createErdosRenyiRandomNetworkAct = new QAction(QIcon(":/images/erdos.png"), tr("Erdős–Rényi"),  this);
    createErdosRenyiRandomNetworkAct -> setShortcut(
                QKeySequence(Qt::CTRL + Qt::Key_R, Qt::CTRL + Qt::Key_E)
                );
    createErdosRenyiRandomNetworkAct->setStatusTip(tr("Creates a random network according to the Erdős–Rényi model"));
    createErdosRenyiRandomNetworkAct->setWhatsThis(
                tr("Erdős–Rényi \n\n") +
                tr("Creates a random network either of G(n, p) model or G(n,M) model.\n") +
                tr("In the first, edges are created with Bernoulli trials (probability p).\n") +
                tr("In the second, a graph of exactly M edges is created."));
    connect(createErdosRenyiRandomNetworkAct, SIGNAL(triggered()), this, SLOT(slotCreateRandomErdosRenyi()));

    createLatticeNetworkAct = new QAction( QIcon(":/images/net1.png"), tr("Ring Lattice"), this);
    createLatticeNetworkAct -> setShortcut(
                QKeySequence(Qt::CTRL + Qt::Key_R, Qt::CTRL + Qt::Key_L)
                );
    createLatticeNetworkAct->setStatusTip(tr("Creates a ring lattice random network"));
    createLatticeNetworkAct->setWhatsThis(
                tr("Ring Lattice \n\n")+
                tr("A ring lattice is a graph with N nodes each connected to d neighbors, d / 2 on each side."));
    connect(createLatticeNetworkAct, SIGNAL(triggered()), this, SLOT(slotCreateRandomRingLattice()));

    createRegularRandomNetworkAct = new QAction(QIcon(":/images/net.png"), tr("d-Regular"), this);
    createRegularRandomNetworkAct -> setShortcut(
                        QKeySequence(Qt::CTRL + Qt::Key_R, Qt::CTRL + Qt::Key_R)
                        );
    createRegularRandomNetworkAct->setStatusTip(tr("Creates a random network where every node has the same degree d."));
    createRegularRandomNetworkAct->setWhatsThis(
                tr("d-Regular \n\n") +
                tr("Creates a random network where each node have the same number of neighbours, aka the same degree d "));
    connect(createRegularRandomNetworkAct, SIGNAL(triggered()), this, SLOT(slotCreateRegularRandomNetwork()));

    createGaussianRandomNetworkAct = new QAction(tr("Gaussian"),	this);
    createGaussianRandomNetworkAct -> setShortcut(
                    QKeySequence(Qt::CTRL + Qt::Key_R, Qt::CTRL + Qt::Key_G)
                    );
    createGaussianRandomNetworkAct->setStatusTip(tr("Creates a Gaussian distributed random network"));
    createGaussianRandomNetworkAct->setWhatsThis(tr("Gaussian \n\nCreates a random network of Gaussian distribution"));
    connect(createGaussianRandomNetworkAct, SIGNAL(triggered()), this, SLOT(slotCreateRandomGaussian()));

    createSmallWorldRandomNetworkAct = new QAction(QIcon(":/images/sw.png"), tr("Small World"),	this);
    createSmallWorldRandomNetworkAct-> setShortcut(
                QKeySequence(Qt::CTRL + Qt::Key_R, Qt::CTRL + Qt::Key_W)
                );
    createSmallWorldRandomNetworkAct->setStatusTip(tr("Creates a random network with small world properties"));
    createSmallWorldRandomNetworkAct ->
            setWhatsThis(
                tr("Small World \n\n") +
                tr("A Small World, according to the Watts and Strogatz model, "
                   "is a random network with short average path lengths and high clustering coefficient."));
    connect(createSmallWorldRandomNetworkAct, SIGNAL(triggered()), this, SLOT(slotCreateRandomSmallWorld()));

    createScaleFreeRandomNetworkAct = new QAction(
                QIcon(":/images/scalefree.png"), tr("Scale-free"),	this);

    createScaleFreeRandomNetworkAct->setShortcut(
                QKeySequence(Qt::CTRL + Qt::Key_R, Qt::CTRL + Qt::Key_S)
                );
    createScaleFreeRandomNetworkAct->setStatusTip(
                tr("Creates a random network with power-law degree distribution."));
    createScaleFreeRandomNetworkAct->
            setWhatsThis(
                tr("Scale-free (power-law)\n\n") +
                tr("A scale-free network is a network whose degree distribution follows a power law."
                   " This method generates random scale-free networks according to the "
                   " Barabási–Albert (BA) model using a preferential attachment mechanism."));
    connect(createScaleFreeRandomNetworkAct, SIGNAL(triggered()), this, SLOT(slotCreateRandomScaleFree()));



    webCrawlerAct = new QAction(QIcon(":/images/webcrawler.png"), tr("Web Crawler"),	this);
    webCrawlerAct->setShortcut(tr("Shift+C"));
    webCrawlerAct->setEnabled(true);
    webCrawlerAct->setStatusTip(tr("Creates a network from all links found in a given website"));
    webCrawlerAct->setWhatsThis(tr("Web Crawler \n\nA Web crawler is a built-in bot, which starts with a given URL (website or webpage) to visit. As the algorithm crawls this webpage, it identifies all the links in the page and adds them to a list of URLs (called frontier). Then, all the URLs from the frontier are recursively visited. You must provide maximum recursion level (how many URLs from the frontier will be visited) and maximum running time, along with the initial web address..."));
    connect(webCrawlerAct, SIGNAL(triggered()), this, SLOT(slotShowWebCrawlerDialog()));


    /**
    Edit menu actions
    */

    selectAllAct = new QAction(QIcon(":/images/selectall.png"), tr("Select All"), this);
    selectAllAct->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_A));
    selectAllAct->setStatusTip(tr("Selects all nodes"));
    selectAllAct->setWhatsThis(tr("Select All\n\nSelects all nodes in the network"));
    connect(selectAllAct, SIGNAL(triggered()), this, SLOT(slotSelectAll()));

    selectNoneAct = new QAction(QIcon(":/images/selectnone.png"), tr("Deselect all"), this);
    selectNoneAct->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_A));
    selectNoneAct->setStatusTip(tr("Deselects all nodes"));
    selectNoneAct->setWhatsThis(tr("Deselect all\n\n Clears the node selection"));
    connect(selectNoneAct, SIGNAL(triggered()), this, SLOT(slotSelectNone()));


    findNodeAct = new QAction(QIcon(":/images/find.png"), tr("Find Node"), this);
    findNodeAct->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_F));
    findNodeAct->setStatusTip(tr("Finds and highlights a node by number or label. Press Ctrl+F again to undo."));
    findNodeAct->setWhatsThis(tr("Find Node\n\nFinds a node with a given number or label and doubles its size. Ctrl+F again resizes back the node"));
    connect(findNodeAct, SIGNAL(triggered()), this, SLOT(slotFindNode()) );

    addNodeAct = new QAction(QIcon(":/images/add.png"), tr("Add Node"), this);
    addNodeAct->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_X, Qt::CTRL + Qt::Key_A));
    addNodeAct->setStatusTip(tr("Adds a node"));
    addNodeAct->setWhatsThis(tr("Add Node\n\nAdds a node to the network"));
    connect(addNodeAct, SIGNAL(triggered()), this, SLOT(addNode()));

    removeNodeAct = new QAction(QIcon(":/images/remove.png"),tr("Remove Node"), this);
    removeNodeAct ->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_X, Qt::CTRL + Qt::Key_Backspace));
    //Single key shortcuts with backspace or del do no work in Mac http://goo.gl/7hz7Dx
    removeNodeAct->setStatusTip(tr("Removes a node"));
    removeNodeAct->setWhatsThis(tr("Remove Node\n\nRemoves a node from the network"));
    connect(removeNodeAct, SIGNAL(triggered()), this, SLOT(slotRemoveNode()));

    propertiesNodeAct = new QAction(QIcon(":/images/properties.png"),tr("Node Properties"), this);
    propertiesNodeAct ->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_X, Qt::CTRL + Qt::Key_P));
    propertiesNodeAct->setStatusTip(tr("Open node properties"));
    propertiesNodeAct->setWhatsThis(tr("Node Properties\n\nOpens node properties to edit label, size, color, shape etc"));
    connect(propertiesNodeAct, SIGNAL(triggered()), this, SLOT(slotChangeNodeProperties()));

    changeAllNodesSizeAct = new QAction(QIcon(":/images/resize.png"), tr("Change all Nodes Size"),	this);
    changeAllNodesSizeAct->setStatusTip(tr("This option lets you change the size of all nodes"));
    changeAllNodesSizeAct->setWhatsThis(tr("Nodes Size\n\nThis option lets you change the size of all nodes"));
    connect(changeAllNodesSizeAct, SIGNAL(triggered()), this, SLOT(slotChangeAllNodesSize()) );

    changeAllNodesShapeAct = new QAction( tr("Change all Nodes Shape"),	this);
    changeAllNodesShapeAct->setStatusTip(tr("This option lets you change the shape of all nodes"));
    changeAllNodesShapeAct->setWhatsThis(tr("Nodes Shape\n\nThis option lets you change the shape of all nodes"));
    connect(changeAllNodesShapeAct, SIGNAL(triggered()), this, SLOT(slotChangeAllNodesShape()) );

    changeNumbersSizeAct = new QAction( tr("Change all Numbers Size"),	this);
    changeNumbersSizeAct->setStatusTip(tr("It lets you change the font size of the numbers of all nodes"));
    changeNumbersSizeAct->setWhatsThis(tr("Numbers Size\n\nChanges the size of the numbers of all nodes"));
    connect(changeNumbersSizeAct, SIGNAL(triggered()), this, SLOT(slotChangeNumbersSize()) );

    changeLabelsSizeAct = new QAction( tr("Change all Labels Size"), this);
    changeLabelsSizeAct->setStatusTip(tr("You can change the font size of the labels of all nodes"));
    changeLabelsSizeAct->setWhatsThis(tr("Labels Size\n\nChange the fontsize of the labels of all nodes"));
    connect(changeLabelsSizeAct, SIGNAL(triggered()), this, SLOT(slotChangeLabelsSize()) );

    addEdgeAct = new QAction(QIcon(":/images/plines.png"), tr("Add Edge"),this);
    addEdgeAct->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_E, Qt::CTRL + Qt::Key_A));
    addEdgeAct->setStatusTip(tr("Adds a directed edge from a node to another"));
    addEdgeAct->setWhatsThis(tr("Add Edge\n\nAdds a directed edge from a node to another"));
    connect(addEdgeAct, SIGNAL(triggered()), this, SLOT(slotAddEdge()));

    removeEdgeAct = new QAction(QIcon(":/images/disconnect.png"), tr("Remove"), this);
    //removeEdgeAct ->setShortcut(QKeySequence(Qt::SHIFT+Qt::Key_Backspace));
    removeEdgeAct ->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_E, Qt::CTRL + Qt::Key_Backspace));
    removeEdgeAct->setStatusTip(tr("Removes an Edge"));
    removeEdgeAct->setWhatsThis(tr("Remove Edge\n\nRemoves an Edge from the network"));
    connect(removeEdgeAct, SIGNAL(triggered()), this, SLOT(slotRemoveEdge()));

    changeEdgeLabelAct = new QAction(QIcon(":/images/letters.png"), tr("Change Label"), this);
    changeEdgeLabelAct->setStatusTip(tr("Changes the Label of an Edge"));
    changeEdgeLabelAct->setWhatsThis(tr("Change Label\n\nChanges the label of an Edge"));
    connect(changeEdgeLabelAct, SIGNAL(triggered()), this, SLOT(slotChangeEdgeLabel()));
    changeEdgeLabelAct->setEnabled(false);

    changeEdgeColorAct = new QAction(QIcon(":/images/colorize.png"),tr("Change Color"),	this);
    changeEdgeColorAct->setStatusTip(tr("Changes the Color of an Edge"));
    changeEdgeColorAct->setWhatsThis(tr("Change Color\n\nChanges the Color of an Edge"));
    connect(changeEdgeColorAct, SIGNAL(triggered()), this, SLOT(slotChangeEdgeColor()));

    changeEdgeWeightAct = new QAction(tr("Change Weight"), this);
    changeEdgeWeightAct->setStatusTip(tr("Changes the Weight of an Edge"));
    changeEdgeWeightAct->setWhatsThis(tr("Change Value\n\nChanges the Weight of an Edge"));
    connect(changeEdgeWeightAct, SIGNAL(triggered()), this, SLOT(slotChangeEdgeWeight()));

    filterNodesAct = new QAction(tr("Filter Nodes"), this);
    filterNodesAct -> setEnabled(false);
    filterNodesAct->setStatusTip(tr("Filters Nodes of some value out of the network"));
    filterNodesAct->setWhatsThis(tr("Filter Nodes\n\nFilters Nodes of some value out of the network."));
    connect(filterNodesAct, SIGNAL(triggered()), this, SLOT(slotFilterNodes()));

    filterIsolateNodesAct = new QAction(tr("Filter Isolate Nodes"), this);
    filterIsolateNodesAct -> setEnabled(true);
    filterIsolateNodesAct -> setCheckable(true);
    filterIsolateNodesAct -> setChecked(false);
    filterIsolateNodesAct -> setStatusTip(tr("Filters nodes with no edges"));
    filterIsolateNodesAct -> setWhatsThis(tr("Filter Isolate Nodes\n\n Enables or disables displaying of isolate nodes. Isolate nodes are those with no edges..."));
    connect(filterIsolateNodesAct, SIGNAL(toggled(bool)), this, SLOT(slotFilterIsolateNodes(bool)));

    filterEdgesAct = new QAction(tr("Filter Edges by weight"), this);
    filterEdgesAct -> setEnabled(true);
    filterEdgesAct -> setStatusTip(tr("Filters Edges of some weight out of the network"));
    filterEdgesAct -> setWhatsThis(tr("Filter Edges\n\nFilters Edge of some specific weight out of the network."));
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

    changeAllEdgesColorAct = new QAction( tr("Change all Edges Colors"), this);
    changeAllEdgesColorAct->setStatusTip(tr("Click to change the color of all Edges."));
    changeAllEdgesColorAct->setWhatsThis(tr("Background\n\nChanges all Edges color"));
    connect(changeAllEdgesColorAct, SIGNAL(triggered()), this, SLOT(slotAllEdgesColor()));



    transformNodes2EdgesAct = new QAction( tr("Transform Nodes to Edges"),this);
    transformNodes2EdgesAct->setStatusTip(tr("Transforms the network so that nodes become Edges and vice versa"));
    transformNodes2EdgesAct->setWhatsThis(tr("Transform Nodes EdgesAct\n\nTransforms network so that nodes become Edges and vice versa"));
    connect(transformNodes2EdgesAct, SIGNAL(triggered()), this, SLOT(slotTransformNodes2Edges()));

    symmetrizeAct= new QAction(QIcon(":/images/symmetrize.png"), tr("Symmetrize Edges"), this);
    symmetrizeAct ->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_E, Qt::CTRL + Qt::Key_S));
    symmetrizeAct->setStatusTip(tr("Makes all edges reciprocal (thus, a symmetric graph)."));
    symmetrizeAct->setWhatsThis(tr("Symmetrize Edges\n\nTransforms all directed arcs to undirected edges. The result is a symmetric network"));
    connect(symmetrizeAct, SIGNAL(triggered()), this, SLOT(slotSymmetrize()));




    /**
    Layout menu actions
    */
    strongColorationAct = new QAction ( tr("Strong Structural"), this);
    strongColorationAct -> setStatusTip( tr("Nodes are assigned the same color if they have identical in and out neighborhoods") );
    strongColorationAct -> setWhatsThis( tr("Click this to colorize nodes; Nodes are assigned the same color if they have identical in and out neighborhoods"));
    connect(strongColorationAct, SIGNAL(triggered() ), this, SLOT(slotColorationStrongStructural()) );

    regularColorationAct = new QAction ( tr("Regular"), this);
    regularColorationAct ->
            setStatusTip(
                tr("Nodes are assigned the same color if they have "
                   "neighborhoods of the same set of colors") );
    regularColorationAct
            -> setWhatsThis(
                tr("Click this to colorize nodes; "
                   "Nodes are assigned the same color if they have neighborhoods "
                   "of the same set of colors"));
    connect(regularColorationAct, SIGNAL(triggered() ), this, SLOT(slotColorationRegular()) );//TODO

    randLayoutAct = new QAction( tr("Random"),this);
    randLayoutAct -> setShortcut(Qt::CTRL+Qt::Key_0);
    randLayoutAct -> setStatusTip(tr("Repositions all nodes in random places"));
    randLayoutAct -> setWhatsThis(tr("Random Layout\n\n Repositions all nodes in random places"));
    connect(randLayoutAct, SIGNAL(triggered()), this, SLOT(slotLayoutRandom()));


    randCircleLayoutAct = new QAction(tr("Random Circles"),	this);
    randCircleLayoutAct -> setShortcut(Qt::CTRL+Qt::ALT+Qt::Key_0);
    randCircleLayoutAct ->setStatusTip(tr("Repositions the nodes randomly on circles"));
    randCircleLayoutAct->
            setWhatsThis(
                tr("Random Circles Layout\n\n Repositions the nodes randomly on circles"));
    connect(randCircleLayoutAct, SIGNAL(triggered()), this, SLOT(slotLayoutCircularRandom()));


    layoutCircular_DC_Act = new QAction( tr("Degree Centrality"),	this);
    layoutCircular_DC_Act ->setShortcut(tr("Ctrl+Alt+1"));
    layoutCircular_DC_Act
            ->setStatusTip(
                tr("Layout all nodes on concentric circles of radius inversely "
                    "proportional to their Degree Centrality."));
    layoutCircular_DC_Act->
            setWhatsThis(
                tr( "Degree Centrality Circular Layout\n\n "
                    "Repositions all nodes on concentric circles of radius "
                    "inversely proportional to their Degree Centrality"
                    "Nodes with higher DC score are closer to the centre."
                    )
                );
    connect(layoutCircular_DC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutCircularByProminenceIndex()) );

    layoutCircular_CC_Act = new QAction( tr("Closeness Centrality"), this);
    layoutCircular_CC_Act ->setShortcut(tr("Ctrl+Alt+2"));
    layoutCircular_CC_Act
            -> setStatusTip(
                tr("Layout all nodes on concentric circles of radius inversely "
                    "proportional to their CC index."));
    layoutCircular_CC_Act->
            setWhatsThis(
                tr( "Closeness Centrality Circular Layout\n\n "
                    "Repositions all nodes on concentric circles of radius "
                    "inversely proportional to their CC index."
                    "Nodes having higher CC score are closer to the centre."
                    ));
    connect(layoutCircular_CC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutCircularByProminenceIndex()));


    layoutCircular_IRCC_Act = new QAction(
                tr("Influence Range Closeness Centrality"),	this);
    layoutCircular_IRCC_Act ->setShortcut(tr("Ctrl+Alt+3"));
    layoutCircular_IRCC_Act
            ->setStatusTip(
                tr(
                   "Layout all nodes on concentric circles of radius inversely "
                    "proportional to their IRCC index."));
    layoutCircular_IRCC_Act->
            setWhatsThis(
                tr(
                    "Influence Range Closeness Centrality Circular Layout\n\n "
                    "Repositions all nodes on concentric circles of radius "
                    "inversely proportional to their IRCC index."
                    "Nodes having higher IRCC score are closer to the centre."
                    ));
    connect(layoutCircular_IRCC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutCircularByProminenceIndex()));

    layoutCircular_BC_Act = new QAction( tr("Betweenness Centrality"), this);
    layoutCircular_BC_Act ->setShortcut(tr("Ctrl+Alt+4"));
    layoutCircular_BC_Act ->setStatusTip(
                tr(
                   "Layout all nodes on concentric circles of radius inversely "
                    "proportional to their BC index."));
    layoutCircular_BC_Act->
            setWhatsThis(
                tr(
                    "Betweenness Centrality Circular Layout\n\n "
                    "Repositions all nodes on concentric circles of radius "
                    "inversely proportional to their BC index."
                    "Nodes having higher BC score are closer to the centre."
                    ));
    connect(layoutCircular_BC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutCircularByProminenceIndex()));

    layoutCircular_SC_Act = new QAction( tr("Stress Centrality"),	this);
    layoutCircular_SC_Act ->setShortcut(tr("Ctrl+Alt+5"));
    layoutCircular_SC_Act ->setStatusTip(
                tr(
                   "Layout all nodes on concentric circles of radius inversely "
                    "proportional to their SC index."));
    layoutCircular_SC_Act->
            setWhatsThis(
                tr(
                    "Stress Centrality Circular Layout\n\n "
                    "Repositions all nodes on concentric circles of radius "
                    "inversely proportional to their SC index."
                    "Nodes having higher SC score are closer to the centre."
                    ));
    connect(layoutCircular_SC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutCircularByProminenceIndex()));

    layoutCircular_EC_Act = new QAction( tr("Eccentricity Centrality"),	this);
    layoutCircular_EC_Act ->setShortcut(tr("Ctrl+Alt+6"));
    layoutCircular_EC_Act ->setStatusTip(
                tr(
                   "Layout all nodes on concentric circles of radius inversely "
                    "proportional to their EC index."));
    layoutCircular_EC_Act->
            setWhatsThis(
                tr(
                    "Eccentricity Centrality Circular Layout\n\n "
                    "Repositions all nodes on concentric circles of radius "
                    "inversely proportional to their EC index."
                    "Nodes having higher EC score are closer to the centre."
                    ));
    connect(layoutCircular_EC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutCircularByProminenceIndex()));


    layoutCircular_PC_Act = new QAction( tr("Power Centrality"),	this);
    layoutCircular_PC_Act ->setShortcut(tr("Ctrl+Alt+7"));
    layoutCircular_PC_Act ->setStatusTip(
                tr(
                   "Layout all nodes on concentric circles of radius inversely "
                    "proportional to their PC index."));
    layoutCircular_PC_Act->
            setWhatsThis(
                tr(
                    "Power Centrality Circular Layout\n\n "
                    "Repositions all nodes on concentric circles of radius "
                    "inversely proportional to their PC index."
                    "Nodes having higher PC score are closer to the centre."
                    ));
    connect(layoutCircular_PC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutCircularByProminenceIndex()));


    layoutCircular_IC_Act = new QAction( tr("Information Centrality"),	this);
    layoutCircular_IC_Act ->setEnabled(true);
    layoutCircular_IC_Act ->setShortcut(tr("Ctrl+Alt+8"));
    layoutCircular_IC_Act ->setStatusTip(
                tr(
                   "Layout all nodes on concentric circles of radius inversely "
                    "proportional to their IC index."));
    layoutCircular_IC_Act->
            setWhatsThis(
                tr(
                    "Information Centrality Circular Layout\n\n "
                    "Repositions all nodes on concentric circles of radius "
                    "inversely proportional to their IC index."
                    "Nodes having higher IC score are closer to the centre."
                    ));
    connect(layoutCircular_IC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutCircularByProminenceIndex()));


    layoutCircular_DP_Act = new QAction( tr("Degree Prestige"),	this);
    layoutCircular_DP_Act ->setShortcut(tr("Ctrl+Alt+I"));
    layoutCircular_DP_Act ->setStatusTip(
                tr(
                   "Layout all nodes on concentric circles of radius inversely "
                    "proportional to their DP index."));
    layoutCircular_DP_Act->
            setWhatsThis(
                tr(
                    "Degree Prestige Circular Layout\n\n "
                    "Repositions all nodes on concentric circles of radius "
                    "inversely proportional to their DP index."
                    "Nodes having higher DP score are closer to the centre."
                    ));
    connect(layoutCircular_DP_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutCircularByProminenceIndex()));

    layoutCircular_PRP_Act = new QAction( tr("PageRank Prestige"),	this);
    layoutCircular_PRP_Act ->setEnabled(true);
    layoutCircular_PRP_Act ->setShortcut(tr("Ctrl+Alt+K"));
    layoutCircular_PRP_Act ->setStatusTip(
                tr(
                   "Layout all nodes on concentric circles of radius inversely "
                    "proportional to their PRP index."));
    layoutCircular_PRP_Act->
            setWhatsThis(
                tr(
                    "PageRank Prestige Circular Layout\n\n "
                    "Repositions all nodes on concentric circles of radius "
                    "inversely proportional to their PRP index."
                    "Nodes having higher PRP score are closer to the centre."
                    ));
    connect(layoutCircular_PRP_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutCircularByProminenceIndex()));


    layoutCircular_PP_Act = new QAction( tr("Proximity Prestige"),	this);
    layoutCircular_PP_Act ->setShortcut(tr("Ctrl+Alt+Y"));
    layoutCircular_PP_Act ->setStatusTip(
                tr(
                   "Layout all nodes on concentric circles of radius inversely "
                    "proportional to their PP index."));
    layoutCircular_PP_Act->
            setWhatsThis(
                tr(
                    "Proximity Prestige Circular Layout\n\n "
                    "Repositions all nodes on concentric circles of radius "
                    "inversely proportional to their PP index."
                    "Nodes having higher PP score are closer to the centre."
                    ));
    connect(layoutCircular_PP_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutCircularByProminenceIndex()));



    clearGuidesAct = new QAction(QIcon(":/images/gridlines.png"), tr("Remove Layout GuideLines"), this);
    clearGuidesAct ->setStatusTip(tr("Removes all layout guideLines from the canvas."));
    clearGuidesAct->setWhatsThis(tr("Remove GuideLines\n\n Removes any guidelines (circles or horizontal lines) created for the network layout."));




    layoutLevel_DC_Act = new QAction( tr("Degree Centrality"),	this);
    layoutLevel_DC_Act ->setShortcut(tr("Ctrl+Shift+1"));
    layoutLevel_DC_Act
            ->setStatusTip(
                tr(
                    "Layout nodes on horizontal levels of height "
                     "proportional to their DC index."));
    layoutLevel_DC_Act->
             setWhatsThis(
                 tr(
                     "Degree Centrality Levels Layout\n\n "
                     "Repositions all nodes on horizontal levels of height"
                     "proportional to their DC index."
                     "Nodes having higher DC score are closer to the top.\n\n"
                    )
                );
    connect(layoutLevel_DC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutLevelByProminenceIndex()) );

    layoutLevel_CC_Act = new QAction( tr("Closeness Centrality"), this);
    layoutLevel_CC_Act ->setShortcut(tr("Ctrl+Shift+2"));
    layoutLevel_CC_Act
            -> setStatusTip(
                tr(
                    "Layout nodes on horizontal levels of height "
                     "proportional to their CC index."));
    layoutLevel_CC_Act->
             setWhatsThis(
                 tr(
                     "Closeness Centrality Levels Layout\n\n "
                     "Repositions all nodes on horizontal levels of height"
                     "proportional to their CC index."
                     "Nodes having higher CC score are closer to the top.\n\n"
                     "This layout can be computed only for connected graphs. "
                    ));
    connect(layoutLevel_CC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutLevelByProminenceIndex()));


    layoutLevel_IRCC_Act = new QAction(
                tr("Influence Range Closeness Centrality"),	this);
    layoutLevel_IRCC_Act ->setShortcut(tr("Ctrl+Shift+3"));
    layoutLevel_IRCC_Act
            ->setStatusTip(
                tr(
                    "Layout nodes on horizontal levels of height "
                     "proportional to their IRCC index."));
    layoutLevel_IRCC_Act->
             setWhatsThis(
                 tr(
                     "Influence Range Closeness Centrality Levels Layout\n\n "
                     "Repositions all nodes on horizontal levels of height"
                     "proportional to their IRCC index."
                     "Nodes having higher IRCC score are closer to the top.\n\n"
                     "This layout can be computed for not connected graphs. "
                    ));
    connect(layoutLevel_IRCC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutLevelByProminenceIndex()));

    layoutLevel_BC_Act = new QAction( tr("Betweenness Centrality"), this);
    layoutLevel_BC_Act ->setShortcut(tr("Ctrl+Shift+4"));
    layoutLevel_BC_Act ->setStatusTip(
                tr(
                    "Layout nodes on horizontal levels of height "
                     "proportional to their BC index."));
    layoutLevel_BC_Act->
             setWhatsThis(
                 tr(
                     "Betweenness Centrality Levels Layout\n\n "
                     "Repositions all nodes on horizontal levels of height"
                     "proportional to their BC index."
                     "Nodes having higher BC score are closer to the top."
                    ));
    connect(layoutLevel_BC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutLevelByProminenceIndex()));

    layoutLevel_SC_Act = new QAction( tr("Stress Centrality"),	this);
    layoutLevel_SC_Act ->setShortcut(tr("Ctrl+Shift+5"));
    layoutLevel_SC_Act ->setStatusTip(
                tr(
                    "Layout nodes on horizontal levels of height "
                     "proportional to their SC index."));
    layoutLevel_SC_Act->
             setWhatsThis(
                 tr(
                     "Stress Centrality Levels Layout\n\n "
                     "Repositions all nodes on horizontal levels of height"
                     "proportional to their SC index."
                     "Nodes having higher SC score are closer to the top."
                    ));
    connect(layoutLevel_SC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutLevelByProminenceIndex()));

    layoutLevel_EC_Act = new QAction( tr("Eccentricity Centrality"),	this);
    layoutLevel_EC_Act ->setShortcut(tr("Ctrl+Shift+6"));
    layoutLevel_EC_Act ->setStatusTip(
                tr(
                    "Layout nodes on horizontal levels of height "
                     "proportional to their EC index."));
    layoutLevel_EC_Act->
             setWhatsThis(
                 tr(
                     "Eccentricity Centrality Levels Layout\n\n "
                     "Repositions all nodes on horizontal levels of height"
                     "proportional to their EC index."
                     "Nodes having higher EC score are closer to the top."
                    ));
    connect(layoutLevel_EC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutLevelByProminenceIndex()));


    layoutLevel_PC_Act = new QAction( tr("Power Centrality"),	this);
    layoutLevel_PC_Act ->setShortcut(tr("Ctrl+Shift+7"));
    layoutLevel_PC_Act ->setStatusTip(
                tr(
                    "Layout nodes on horizontal levels of height "
                     "proportional to their PC index."));
    layoutLevel_PC_Act->
             setWhatsThis(
                 tr(
                     "Power Centrality Levels Layout\n\n "
                     "Repositions all nodes on horizontal levels of height"
                     "proportional to their PC index."
                     "Nodes having higher PC score are closer to the top."
                    ));
    connect(layoutLevel_PC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutLevelByProminenceIndex()));


    layoutLevel_IC_Act = new QAction( tr("Information Centrality"),	this);
    layoutLevel_IC_Act ->setEnabled(true);
    layoutLevel_IC_Act ->setShortcut(tr("Ctrl+Shift+8"));
    layoutLevel_IC_Act ->setStatusTip(
                tr(
                    "Layout nodes on horizontal levels of height "
                     "proportional to their IC index."));
    layoutLevel_IC_Act->
             setWhatsThis(
                 tr(
                     "Information Centrality Levels Layout\n\n "
                     "Repositions all nodes on horizontal levels of height"
                     "proportional to their IC index."
                     "Nodes having higher IC score are closer to the top."
                    ));
    connect(layoutLevel_IC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutLevelByProminenceIndex()));


    layoutLevel_DP_Act = new QAction( tr("Degree Prestige"),	this);
    layoutLevel_DP_Act ->setShortcut(tr("Ctrl+Shift+I"));
    layoutLevel_DP_Act ->setStatusTip(
                tr(
                   "Layout nodes on horizontal levels of height "
                    "proportional to their DP index."));
    layoutLevel_DP_Act->
            setWhatsThis(
                tr(
                    "Degree Prestige Levels Layout\n\n "
                    "Repositions all nodes on horizontal levels of height"
                    "proportional to their DP index."
                    "Nodes having higher DP score are closer to the top."
                    ));
    connect(layoutLevel_DP_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutLevelByProminenceIndex()));

    layoutLevel_PRP_Act = new QAction( tr("PageRank Prestige"),	this);
    layoutLevel_PRP_Act ->setEnabled(true);
    layoutLevel_PRP_Act ->setShortcut(tr("Ctrl+Shift+K"));
    layoutLevel_PRP_Act ->setStatusTip(
                tr(
                   "Layout nodes on horizontal levels of height "
                    "proportional to their PRP index."));
    layoutLevel_PRP_Act->
            setWhatsThis(
                tr(
                    "PageRank Prestige Levels Layout\n\n "
                    "Repositions all nodes on horizontal levels of height"
                    "proportional to their PRP index."
                    "Nodes having higher PRP score are closer to the top."
                    ));
    connect(layoutLevel_PRP_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutLevelByProminenceIndex()));


    layoutLevel_PP_Act = new QAction( tr("Proximity Prestige"),	this);
    layoutLevel_PP_Act ->setEnabled(true);
    layoutLevel_PP_Act ->setShortcut(tr("Ctrl+Shift+Y"));
    layoutLevel_PP_Act ->setStatusTip(
                tr(
                   "Layout nodes on horizontal levels of height "
                    "proportional to their PP index."));
    layoutLevel_PP_Act->
            setWhatsThis(
                tr(
                    "Proximity Prestige Levels Layout\n\n "
                    "Repositions all nodes on horizontal levels of height"
                    "proportional to their PP index."
                    "Nodes having higher PP score are closer to the top."
                    ));
    connect(layoutLevel_PP_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutLevelByProminenceIndex()));


    springLayoutAct= new QAction(tr("Spring Embedder (Eades)"), this);
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
    zoomInAct->setShortcut(Qt::CTRL + Qt::Key_Plus);
    zoomInAct->setToolTip(tr("Zoom in (Ctrl++)"));
    zoomInAct->setStatusTip(tr("Zooms inside the actual network."));
    zoomInAct->setWhatsThis(tr("Zoom In.\n\nZooms in. What else did you expect?"));

    zoomOutAct = new QAction(QIcon(":/images/zoomout.png"), tr("Zoom &out"),  this);
    zoomOutAct->setShortcut(Qt::CTRL + Qt::Key_Minus);
    zoomOutAct->setToolTip(tr("Zoom out (Ctrl+-)"));
    zoomOutAct->setStatusTip(tr("Zooms out of the actual network."));
    zoomOutAct->setWhatsThis(tr("Zoom out.\n\nZooms out. What else did you expect?"));


    nextRelationAct = new QAction(QIcon(":/images/nextrelation.png"),
                                  tr("Next Relation"),  this);
    nextRelationAct->setShortcut(Qt::CTRL + Qt::Key_Right);
    nextRelationAct->setToolTip(tr("Goto next graph relation (Ctrl+Right)"));
    nextRelationAct->setStatusTip(tr("Loads the next relation of the network (if any)."));
    nextRelationAct->setWhatsThis(tr("Next Relation\n\nLoads the next relation of the network (if any)"));

    prevRelationAct = new QAction(QIcon(":/images/prevrelation.png"),
                                      tr("Previous Relation"),  this);
    prevRelationAct->setShortcut(Qt::CTRL + Qt::Key_Left);
    prevRelationAct->setToolTip(
                tr("Goto previous graph relation (Ctrl+Left)"));
    prevRelationAct->setStatusTip(
                tr("Loads the previous relation of the network (if any)."));
    prevRelationAct->setWhatsThis(
                tr("Previous Relation\n\n"
                   "Loads the previous relation of the network (if any)"));

    addRelationAct = new QAction(QIcon(":/images/addrelation.png"),
                                      tr("Add New Relation"),  this);
    addRelationAct->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_N);
    addRelationAct->setToolTip(
                tr("Add a new relation to the active graph (Ctrl+Shift+N)"));
    addRelationAct->setStatusTip(
                tr("Adds a new relation to the network. "
                   "Nodes will be preserved, edges will be removed. "));
    addRelationAct->setWhatsThis(
                tr("Add New Relation\n\n"
                   "Adds a new relation to the active network. "
                   "Nodes will be preserved, edges will be removed. "));

    nodeSizesByOutDegreeAct= new QAction(QIcon(":/images/nodeout.png"),
                                         tr("Node sizes by OutDegree"), this);
    nodeSizesByOutDegreeAct->setShortcut(tr("Alt+3"));
    nodeSizesByOutDegreeAct->
            setStatusTip(tr("Resizes all nodes according to their outDegree."));
    nodeSizesByOutDegreeAct
            ->setWhatsThis(tr("Node sizes by OutDegree) \n\n"
                              "Adjusts the size of each node according to its "
                              "OutDegree. The more out-linked a node is, "
                              "the bigger will appear..."));
    nodeSizesByOutDegreeAct->setCheckable(true);
    nodeSizesByOutDegreeAct->setChecked(false);
    connect(nodeSizesByOutDegreeAct, SIGNAL(triggered(bool)),
            this, SLOT(slotLayoutNodeSizesByOutDegree(bool)));

    nodeSizesByInDegreeAct= new QAction(
                QIcon(":/images/nodein.png"),tr("Node sizes by InDegree"), this);
    nodeSizesByInDegreeAct->setShortcut(tr("Alt+4"));
    nodeSizesByInDegreeAct->setStatusTip(
                tr("Resizes all nodes according to their InDegree."));
    nodeSizesByInDegreeAct->
            setWhatsThis(tr("Node sizes by InDegree) \n\n "
                            "This method adjusts the size of each node according "
                            "to its InDegree. The more in-linked a node is, "
                            "the bigger will appear..."));
    nodeSizesByInDegreeAct->setCheckable(true);
    nodeSizesByInDegreeAct->setChecked(false);
    connect(nodeSizesByInDegreeAct, SIGNAL(triggered(bool)),
            this, SLOT(slotLayoutNodeSizesByInDegree(bool)));


    /**
    Analysis menu actions
    */

    symmetryAct = new QAction(
                QIcon(":/images/symmetry.png"), tr("Symmetry Test"), this);
    symmetryAct ->setShortcut(tr("Shift+S"));
    symmetryAct->setStatusTip(tr("Checks whether the network is symmetric or not"));
    symmetryAct->setWhatsThis(
                tr("Symmetry\n\n "
                   "Checks whether the network is symmetric or not. \n"
                   "A network is symmetric when all edges are reciprocal, or, "
                   "in mathematical language, when the adjacency matrix is "
                   "symmetric.")
                );
    connect(symmetryAct, SIGNAL(triggered()), this, SLOT(slotCheckSymmetry()));

    invertAdjMatrixAct = new QAction(
                QIcon(":/images/symmetry.png"), tr("Invert Adjacency Matrix"), this);
    invertAdjMatrixAct ->setShortcut(tr("Shift+I"));
    invertAdjMatrixAct->setStatusTip(tr("Inverts the adjacency matrix"));
    invertAdjMatrixAct->setWhatsThis(tr("Invert  Adjacency Matrix \n\n Inverts the adjacency matrix using linear algebra methods."));
    connect(invertAdjMatrixAct, SIGNAL(triggered()), this, SLOT(slotInvertAdjMatrix()));

    graphDistanceAct = new QAction(
                QIcon(":/images/distance.png"), tr("Distance"), this
                );
    graphDistanceAct ->setShortcut(tr("Ctrl+G"));
    graphDistanceAct->setStatusTip(
                tr("Calculates the length of the shortest path between two nodes..."));
    graphDistanceAct->setWhatsThis(
                tr("Distance\n\n "
                   "In graph theory, the distance (geodesic distance) of two "
                   "nodes is the length (number of edges) of the shortest path "
                   "between them."));
    connect(graphDistanceAct, SIGNAL(triggered()), this, SLOT(slotGraphDistance()));


    distanceMatrixAct = new QAction(QIcon(":/images/dm.png"), tr("Distances Matrix"),this);
    distanceMatrixAct ->setShortcut(tr("Ctrl+Shift+G"));
    distanceMatrixAct->
            setStatusTip(
                tr("The matrix of geodesic distances between all pair of nodes.")
                );
    distanceMatrixAct->
            setWhatsThis(
                tr("Distances Matrix\n\n"
                   "Calculates and displays the matrix of distances between all "
                   "possible pair of nodes in the social network."
                   "A distances matrix is a n x n square matrix, in which the "
                   "(i,j) element is the distance from node i to node j"
                   "The distance of two nodes is the length of the shortest path between them.")
                );
    connect(distanceMatrixAct, SIGNAL(triggered()), this, SLOT( slotDistancesMatrix() ) );

    geodesicsMatrixAct = new QAction(QIcon(":/images/dm.png"), tr("Geodesics Matrix"),this);
    geodesicsMatrixAct ->setShortcut(tr("Ctrl+Alt+G"));
    geodesicsMatrixAct->setStatusTip(tr("The number of geodesic paths between each pair of nodes "));
    geodesicsMatrixAct->setWhatsThis(
                tr(
                    "Geodesics Matrix\n\n"
                    "Displays a n x n square matrix, where the (i,j) element "
                    "is the number of geodesics between node i and node j. "
                    "A geodesic of two nodes is the shortest path between them.")
                );
    connect(geodesicsMatrixAct, SIGNAL(triggered()),
            this, SLOT( slotGeodesicsMatrix()) );

    diameterAct = new QAction(QIcon(":/images/diameter.png"), tr("Diameter"),this);
    diameterAct ->setShortcut(tr("Ctrl+D"));
    diameterAct->setStatusTip(tr("The diameter of the network."));
    diameterAct->setWhatsThis(tr("Diameter\n\n The Diameter of a network is the maximum graph distance (maximum shortest path length) between any two nodes of the network."));
    connect(diameterAct, SIGNAL(triggered()), this, SLOT(slotDiameter()));

    averGraphDistanceAct = new QAction(QIcon(":/images/avdistance.png"), tr("Average Distance"),this);
    averGraphDistanceAct ->setShortcut(tr("Ctrl+B"));
    averGraphDistanceAct->setStatusTip(tr("The average shortest path length."));
    averGraphDistanceAct->setWhatsThis(tr("Average Distance\n\n This the average length of all shortest paths (geodesics) between the connected pair of nodes of the network."));
    connect(averGraphDistanceAct, SIGNAL(triggered()),
            this, SLOT(slotAverageGraphDistance()));

    eccentricityAct = new QAction(QIcon(":/images/eccentricity.png"), tr("Eccentricity"),this);
    eccentricityAct->setShortcut(tr(""));
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
                                      "from j to i for all pairs (i,j).\n"
                                      "A digraph is weakly connected if at least "
                                      "a pair of nodes are joined by a semipath.\n"
                                      "A digraph or a graph is disconnected if "
                                      "at least one node is isolate."
                                      ));
    connect(connectednessAct, SIGNAL(triggered()), this, SLOT(slotConnectedness()));


    walksAct = new QAction(QIcon(":/images/walk.png"), tr("Walks of a given length"),this);
    walksAct->setShortcut(tr("Ctrl+W"));
    walksAct->setStatusTip(tr("The number of walks of a given length between any nodes."));
    walksAct->setWhatsThis(tr("Walks of a given length\n\n A walk is a sequence of alternating vertices and edges such as v<sub>0</sub>e<sub>1</sub>, v<sub>1</sub>e<sub>2</sub>, v<sub>2</sub>e<sub>3</sub>, …, e<sub>k</sub>v<sub>k</sub>, where each edge, e<sub>i</sub> is defined as e<sub>i</sub> = {v<sub>i-1</sub>, v<sub>i</sub>}. This function counts the number of walks of a given length between each pair of nodes, by studying the powers of the sociomatrix.\n "));
    connect(walksAct, SIGNAL(triggered()), this, SLOT(slotWalksOfGivenLength() )  );

    totalWalksAct = new QAction(QIcon(":/images/walk.png"), tr("Total Walks"),this);
    totalWalksAct->setShortcut(tr("Ctrl+Shift+W"));
    totalWalksAct->setStatusTip(tr("Calculates the total number of walks of every possible length between all nodes"));
    totalWalksAct->setWhatsThis(tr("Total Walks\n\n A walk is a sequence of alternating vertices and edges such as v<sub>0</sub>e<sub>1</sub>, v<sub>1</sub>e<sub>2</sub>, v<sub>2</sub>e<sub>3</sub>, …, e<sub>k</sub>v<sub>k</sub>, where each edge, e<sub>i</sub> is defined as e<sub>i</sub> = {v<sub>i-1</sub>, v<sub>i</sub>}. This function counts the number of walks of any length between each pair of nodes, by studying the powers of the sociomatrix\n "));
    connect(totalWalksAct, SIGNAL(triggered()), this, SLOT(slotTotalWalks() )  );


    reachabilityMatrixAct = new QAction(QIcon(":/images/walk.png"), tr("Reachability Matrix"),this);
    reachabilityMatrixAct->setShortcut(tr("Ctrl+Shift+R"));
    reachabilityMatrixAct->setStatusTip(tr("Calculates the Reachability Matrix for the loaded network."));
    reachabilityMatrixAct->setWhatsThis(tr("Reachability Matrix\n\n     Calculates the reachability matrix X<sup>R</sup> of the graph where the {i,j} element is 1 if the vertices i and j are reachable. \n\n Actually, this just checks whether the corresponding element of Distances matrix is not zero.\n "));
    connect(reachabilityMatrixAct, SIGNAL(triggered()), this, SLOT(slotReachabilityMatrix() )  );

    cliquesAct = new QAction(QIcon(":/images/clique.png"), tr("Clique Census (clique number <= 4) "),this);
    cliquesAct->setShortcut(tr("Ctrl+T"));
    cliquesAct->setStatusTip(tr("Computes a partial clique census report (for cliques up to 4 vertices)."));
    cliquesAct->setWhatsThis(tr("Clique Census\n\n Computes aggregate counts of cliques (up to clique number 4), along with disaggregation by vertex and co-membership information. "));
    connect(cliquesAct, SIGNAL(triggered()), this, SLOT(slotCliqueCensus() )  );


    clusteringCoefAct = new QAction(QIcon(":/images/clique.png"), tr("Clustering Coefficient"),this);
    clusteringCoefAct ->setShortcut(tr("Ctrl+C"));
    clusteringCoefAct->setStatusTip(tr("The average Clustering Coefficient of the network."));
    clusteringCoefAct->setWhatsThis(tr("Clustering Coefficient\n\n The Clustering Coefficient of a vertex quantifies how close the vertex and its neighbors are to being a clique. \n "));
    connect(clusteringCoefAct, SIGNAL(triggered()), this, SLOT(slotClusteringCoefficient() )  );


    triadCensusAct = new QAction(QIcon(":/images/triad.png"), tr("Triad Census"),this);
    triadCensusAct->setShortcut(tr("Ctrl+Shift+T"));
    triadCensusAct->setStatusTip(tr("Conducts a triad census for the active network."));
    triadCensusAct->setWhatsThis(tr("Triad Census\n\n A triad census counts all the different kinds of observed triads within a network and codes them according to their number of mutual, asymmetric and non-existent dyads. \n "));
    connect(triadCensusAct, SIGNAL(triggered()), this, SLOT(slotTriadCensus() )  );

    cDegreeAct = new QAction(tr("Degree Centrality (DC)"),this);
    cDegreeAct->setShortcut(tr("Ctrl+1"));
    cDegreeAct
            ->setStatusTip(tr("Degree Centrality indices and group Degree Centralization."));
    cDegreeAct
            ->setWhatsThis(
                tr( "Degree Centrality (DC)\n\n "
                    "For each node v, the DC index is the number of edges "
                    "attached to it (in undirected graphs) or the total number "
                    "of arcs (outLinks) starting from it (in digraphs).\n"
                    "This is often considered a measure of actor activity. \n\n"
                    "This index can be calculated in both graphs and digraphs "
                    "but is usually best suited for undirected graphs. "
                    "It can also be calculated in weighted graphs. "
                    "In weighted relations, DC is the sum of weights of all "
                    "edges/outLinks attached to v."));
    connect(cDegreeAct, SIGNAL(triggered()), this, SLOT(slotCentralityDegree()));


    cClosenessAct = new QAction(tr("Closeness Centrality (CC)"), this);
    cClosenessAct->setShortcut(tr("Ctrl+2"));
    cClosenessAct
            ->setStatusTip(
                tr(
                    "Closeness Centrality indices and group Closeness Centralization."));
    cClosenessAct
            ->setWhatsThis(
                tr("Closeness Centrality (CC)\n\n "
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
    cInfluenceRangeClosenessAct
            ->setStatusTip(
                tr("Closeness Centrality indices focusing on how proximate each node is"
                   "to the nodes in its influence range"));
    cInfluenceRangeClosenessAct
            ->setWhatsThis(
                tr("Influence Range Closeness Centrality (IRCC)\n\n "
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

    cBetweennessAct = new QAction(tr("Betweenness Centrality (BC)"), this);
    cBetweennessAct->setShortcut(tr("Ctrl+4"));
    cBetweennessAct->setWhatsThis(tr("Betweenness Centrality (BC)\n\n For each node v, BC is the ratio of all geodesics between pairs of nodes which run through v. It reflects how often an node lies on the geodesics between the other nodes of the network. It can be interpreted as a measure of control. A node which lies between many others is assumed to have a higher likelihood of being able to control information flow in the network. \n\n Note that betweenness centrality assumes that all geodesics have equal weight or are equally likely to be chosen for the flow of information between any two nodes. This is reasonable only on \"regular\" networks where all nodes have similar degrees. On networks with significant degree variance you might want to try informational centrality instead. \n\nThis index can be calculated in both graphs and digraphs but is usually best suited for undirected graphs. It can also be calculated in weighted graphs although the weight of each edge (v,u) in E is always considered to be 1."));
    cBetweennessAct->setStatusTip(tr("Betweenness Centrality indices and group Betweenness Centralization."));
    connect(cBetweennessAct, SIGNAL(triggered()), this, SLOT(slotCentralityBetweenness()));

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
    cInDegreeAct->setShortcut(tr("Ctrl+I"));
    cInDegreeAct->setWhatsThis(tr("InDegree (Degree Prestige)\n\n For each node k, this the number of arcs ending at k. Nodes with higher in-degree are considered more prominent among others. In directed graphs, this index measures the prestige of each node/actor. Thus it is called Degree Prestige. Nodes who are prestigious tend to receive many nominations or choices (in-links). The largest the index is, the more prestigious is the node. \n\nThis index can be calculated only for digraphs. In weighted relations, DP is the sum of weights of all arcs/inLinks ending at node v."));
    connect(cInDegreeAct, SIGNAL(triggered()), this, SLOT(slotPrestigeDegree()));

    cPageRankAct = new QAction(tr("PageRank Prestige (PRP)"),	this);
    cPageRankAct->setShortcut(tr("Ctrl+K"));
    cPageRankAct->setEnabled(true);
    cPageRankAct->setStatusTip(tr("Calculate and display PageRank Prestige"));
    cPageRankAct->setWhatsThis(tr("PageRank Prestige\n\n An importance ranking for each node based on the link structure of the network. PageRank, developed by Page and Brin (1997), focuses on how nodes are connected to each other, treating each edge from a node as a citation/backlink/vote to another. In essence, for each node PageRank counts all backlinks to it, but it does so by not counting all edges equally while it normalizes each edge from a node by the total number of edges from it. PageRank is calculated iteratively and it corresponds to the principal eigenvector of the normalized link matrix. \n\nThis index can be calculated in both graphs and digraphs but is usually best suited for directed graphs since it is a prestige measure. It can also be calculated in weighted graphs. In weighted relations, each backlink to a node v from another node u is considered to have weight=1 but it is normalized by the sum of outLinks weights (outDegree) of u. Therefore, nodes with high outLink weights give smaller percentage of their PR to node v."));
    connect(cPageRankAct, SIGNAL(triggered()), this, SLOT(slotPrestigePageRank()));

    cProximityPrestigeAct = new QAction(tr("Proximity Prestige (PP)"),	this);
    cProximityPrestigeAct->setShortcut(tr("Ctrl+Y"));
    cProximityPrestigeAct->setEnabled(true);
    cProximityPrestigeAct->setStatusTip(tr("Calculate and display Proximity Prestige (digraphs only)"));
    cProximityPrestigeAct
            ->setWhatsThis(
                tr("Proximity Prestige (PP) \n\n "
                   "This index measures how proximate a node v is to the nodes "
                   "in its influence domain I (the influence domain I of a node "
                   "is the number of other nodes that can reach it).\n "
                   "In PP calculation, proximity is based on distances to rather "
                   "than distances from node v. \n"
                   "To put it simply, in PP what matters is how close are all "
                   "the other nodes to node v. \n\n"
                   "The algorithm takes the average distance to node v of all "
                   "nodes in its influence domain, standardizes it by "
                   "multiplying with (N-1)/I and takes its reciprocal. "
                   "In essence, the formula SocNetV uses to calculate PP "
                   "is the ratio of the fraction of nodes that can reach node v, "
                   "to the average distance of that nodes to v: \n"
                   "PP = (I/(N-1))/(sum{d(u,v)}/I) \n"
                   "where the sum is over all nodes in I."));
    connect(cProximityPrestigeAct, SIGNAL(triggered()), this, SLOT(slotPrestigeProximity()));

    /**
    Options menu actions
    */
    displayNodeNumbersAct = new QAction( tr("Display Numbers"), this );
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


    displayEdgesAct = new QAction(tr("Display Edges"),	this);
    displayEdgesAct->setStatusTip(tr("Toggle to display or not Edges"));
    displayEdgesAct->setWhatsThis(tr("Display Edges\n\nClick to enable or disable displaying of Edges"));
    displayEdgesAct->setCheckable(true);
    displayEdgesAct->setChecked(true);
    connect(displayEdgesAct, SIGNAL(toggled(bool)), this, SLOT(slotDisplayEdges(bool)) );

    displayEdgesWeightNumbersAct = new QAction(tr("Display Edge Weights"),	this);
    displayEdgesWeightNumbersAct->setStatusTip(tr("Toggles displaying of numbers of Edges weights"));
    displayEdgesWeightNumbersAct->setWhatsThis(tr("Display Weight Numbers\n\nClick to enable or disable displaying numbers of Edges weight"));
    displayEdgesWeightNumbersAct->setCheckable(true);
    displayEdgesWeightNumbersAct->setChecked(false);
    connect(displayEdgesWeightNumbersAct, SIGNAL(toggled(bool)),
            this, SLOT(slotDisplayEdgesWeightNumbers(bool)) );

    considerEdgeWeightsAct = new QAction(tr("Consider Weights in calculcations"),	this);
    considerEdgeWeightsAct->
            setStatusTip(
                tr("Toggles considering Edge weights during calculations (i.e. distances, centrality, etc)"));
    considerEdgeWeightsAct->
            setWhatsThis(
                tr("Display Weight Numbers\n\n"
                   "Click to enable or disable considering edge weights during "
                   "calculations (i.e. distances, centrality, etc)"));
    considerEdgeWeightsAct->setCheckable(true);
    considerEdgeWeightsAct->setChecked(false);
    connect(considerEdgeWeightsAct, SIGNAL(triggered(bool)),
            this, SLOT(slotConsiderEdgeWeights(bool)) );

    displayEdgesArrowsAct = new QAction( tr("Display Arrows"),this);
    displayEdgesArrowsAct->setStatusTip(tr("Toggles displaying of arrows on edges"));
    displayEdgesArrowsAct->setWhatsThis(tr("Display Arrows\n\nClick to enable or disable displaying of arrows on edges"));
    displayEdgesArrowsAct->setCheckable(true);
    displayEdgesArrowsAct->setChecked(true);
    connect(displayEdgesArrowsAct, SIGNAL(toggled(bool)), this, SLOT(slotDisplayEdgesArrows(bool)) );

    drawEdgesWeightsAct = new QAction( tr("Thickness=Weight"), this);
    drawEdgesWeightsAct->setStatusTip(tr("Draws edges as thick as their weights (if specified)"));
    drawEdgesWeightsAct->setWhatsThis(tr("Draw As Thick As Weights\n\nClick to toggle having all edges as thick as their weight (if specified)"));
    drawEdgesWeightsAct->setCheckable(true);
    drawEdgesWeightsAct->setChecked(false);
    drawEdgesWeightsAct->setEnabled(false);
    connect(drawEdgesWeightsAct, SIGNAL(toggled(bool)), this, SLOT(slotDrawEdgesThickAsWeights()) );

    drawEdgesBezier = new QAction( tr("Bezier Curves"),	this);
    drawEdgesBezier->setStatusTip(tr("Draws Edges as Bezier curves"));
    drawEdgesBezier->setWhatsThis(tr("Edges Bezier\n\nEnables/Disables drawing Edges as Bezier curves."));
    drawEdgesBezier->setCheckable(true);
    drawEdgesBezier->setChecked (false);
    drawEdgesBezier->setEnabled(false);
    connect(drawEdgesBezier, SIGNAL(toggled(bool)), this, SLOT(slotDrawEdgesBezier(bool)) );


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
    importSubMenu -> setIcon(QIcon(":/images/import.png"));
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
    randomNetworkMenu -> setIcon(QIcon(":/images/random.png"));
    networkMenu ->addMenu (randomNetworkMenu);

    randomNetworkMenu -> addAction (createScaleFreeRandomNetworkAct);
    randomNetworkMenu -> addAction (createSmallWorldRandomNetworkAct);
    randomNetworkMenu -> addAction (createErdosRenyiRandomNetworkAct );
    // createGaussianRandomNetworkAct -> addTo(randomNetworkMenu);
    randomNetworkMenu -> addAction (createLatticeNetworkAct);
    randomNetworkMenu -> addAction (createRegularRandomNetworkAct);
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
    editNodeMenu -> addAction (selectAllAct);
    editNodeMenu -> addAction (selectNoneAct);
    editNodeMenu -> addSeparator();
    editNodeMenu -> addAction (findNodeAct);
    editNodeMenu -> addAction (addNodeAct);
    editNodeMenu -> addAction (removeNodeAct);
    editNodeMenu -> addSeparator();
    editNodeMenu -> addAction (propertiesNodeAct);
    editNodeMenu -> addSeparator();
    editNodeMenu -> addAction (changeAllNodesSizeAct);
    editNodeMenu -> addAction (changeAllNodesShapeAct);
    editNodeMenu -> addAction (changeNumbersSizeAct);
    editNodeMenu -> addAction (changeLabelsSizeAct);

    editEdgeMenu = new QMenu(tr("Edge..."));
    editEdgeMenu -> setIcon(QIcon(":/images/line.png"));
    editMenu-> addMenu (editEdgeMenu);
    editEdgeMenu -> addAction(addEdgeAct);
    editEdgeMenu -> addAction(removeEdgeAct);
    editEdgeMenu  ->addSeparator();
    editEdgeMenu -> addAction(changeEdgeLabelAct);
    editEdgeMenu -> addAction(changeEdgeColorAct);
    editEdgeMenu -> addAction(changeEdgeWeightAct);
    editEdgeMenu  ->addSeparator();
    //   transformNodes2EdgesAct -> addTo (editMenu);
    editEdgeMenu  -> addAction (symmetrizeAct);


    editMenu ->addSeparator();
    filterMenu = new QMenu ( tr("Filter..."));
    filterMenu -> setIcon(QIcon(":/images/filter.png"));
    editMenu ->addMenu(filterMenu);

    filterMenu -> addAction(filterNodesAct );
    filterMenu -> addAction(filterIsolateNodesAct );
    filterMenu -> addAction(filterEdgesAct );


    editNodeMenu -> addSeparator();
    colorOptionsMenu=new QMenu(tr("Colors"));
    colorOptionsMenu -> setIcon(QIcon(":/images/colorize.png"));
    editMenu -> addMenu (colorOptionsMenu);
    colorOptionsMenu -> addAction (changeBackColorAct);
    colorOptionsMenu -> addAction (changeAllNodesColorAct);
    colorOptionsMenu -> addAction (changeAllEdgesColorAct);
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

    circleLayoutMenu = new QMenu(tr("Circular by prominence index..."));
    circleLayoutMenu -> setIcon(QIcon(":/images/circular.png"));
    layoutMenu -> addMenu (circleLayoutMenu);
    circleLayoutMenu -> addAction (layoutCircular_DC_Act);
    circleLayoutMenu -> addAction (layoutCircular_CC_Act);
    circleLayoutMenu -> addAction (layoutCircular_IRCC_Act);
    circleLayoutMenu -> addAction (layoutCircular_BC_Act);
    circleLayoutMenu -> addAction (layoutCircular_SC_Act);
    circleLayoutMenu -> addAction (layoutCircular_EC_Act);
    circleLayoutMenu -> addAction (layoutCircular_PC_Act);
    circleLayoutMenu -> addAction (layoutCircular_IC_Act);
    circleLayoutMenu -> addAction (layoutCircular_DP_Act);
    circleLayoutMenu -> addAction (layoutCircular_PRP_Act);
    circleLayoutMenu -> addAction (layoutCircular_PP_Act);

    levelLayoutMenu = new QMenu (tr("On levels by prominence index..."));
    levelLayoutMenu -> setIcon(QIcon(":/images/net3.png"));
    layoutMenu -> addMenu (levelLayoutMenu);
    levelLayoutMenu -> addAction (layoutLevel_DC_Act);
    levelLayoutMenu -> addAction (layoutLevel_CC_Act);
    levelLayoutMenu -> addAction (layoutLevel_IRCC_Act);
    levelLayoutMenu -> addAction (layoutLevel_BC_Act);
    levelLayoutMenu -> addAction (layoutLevel_SC_Act);
    levelLayoutMenu -> addAction (layoutLevel_EC_Act);
    levelLayoutMenu -> addAction (layoutLevel_PC_Act);
    levelLayoutMenu -> addAction (layoutLevel_IC_Act);
    levelLayoutMenu -> addAction (layoutLevel_DP_Act);
    levelLayoutMenu -> addAction (layoutLevel_PRP_Act);
    levelLayoutMenu -> addAction (layoutLevel_PP_Act);

    layoutMenu->addSeparator();
    physicalLayoutMenu = new QMenu (tr("Force-Directed..."));
    physicalLayoutMenu -> setIcon(QIcon(":/images/force.png"));
    layoutMenu -> addMenu (physicalLayoutMenu);
    physicalLayoutMenu -> addAction (springLayoutAct);
    physicalLayoutMenu -> addAction (FRLayoutAct);
    layoutMenu->addSeparator();
    layoutMenu->addAction(nodeSizesByOutDegreeAct);
    layoutMenu->addAction(nodeSizesByInDegreeAct);
    layoutMenu->addSeparator();
    layoutMenu -> addAction (clearGuidesAct);



    /** menuBar entry: statistics menu */
    statMenu = menuBar()->addMenu(tr("&Analyze"));
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
    centrlMenu -> addAction (cBetweennessAct);
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

    edgeOptionsMenu=new QMenu(tr("Edges..."));
    edgeOptionsMenu -> setIcon(QIcon(":/images/line.png"));

    optionsMenu -> addMenu (edgeOptionsMenu);
    edgeOptionsMenu -> addAction (displayEdgesAct);
    edgeOptionsMenu -> addAction (displayEdgesWeightNumbersAct);
    edgeOptionsMenu -> addAction (considerEdgeWeightsAct);
    edgeOptionsMenu -> addAction (displayEdgesArrowsAct );
    edgeOptionsMenu -> addSeparator();
    edgeOptionsMenu -> addAction (drawEdgesWeightsAct);
    edgeOptionsMenu -> addAction (drawEdgesBezier);

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

    toolBar -> addWidget(labelRotateSpinBox);
    toolBar -> addWidget(rotateSpinBox);

    toolBar -> addSeparator();

    //Create relation select widget
    QLabel *labelRelationSelect= new QLabel;
    labelRelationSelect ->setText(tr("Relation:"));
    toolBar -> addWidget (labelRelationSelect);
    toolBar -> addAction (prevRelationAct);
    changeRelationCombo = new QComboBox;
    changeRelationCombo->setCurrentIndex(0);
    toolBar -> addWidget(changeRelationCombo);
    toolBar -> addAction (nextRelationAct);
    toolBar -> addAction (addRelationAct);

    toolBar -> addSeparator();
    toolBar -> addAction ( QWhatsThis::createAction (this));

}







//Creates a dock widget for instant menu access
void MainWindow::initToolBox(){
    toolBox = new QTabWidget;
    //toolBox->setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Ignored));

    /*
     *  create widgets for the Controls Tab
     */

    // create 4 buttons for the Edit groupbox
    addNodeBt= new QPushButton(QIcon(":/images/add.png"),tr("&Add Node"));
    addNodeBt->setFocusPolicy(Qt::NoFocus);
    addNodeBt->setToolTip(
                tr("Add a new node to the network (Ctrl+X, Ctrl+A). \n\n "
                   "Alternately, you can create a new node \n"
                   "in a specific position by double-clicking \n"
                   "on that spot of the canvas.")
                );

    removeNodeBt= new QPushButton(QIcon(":/images/remove.png"),tr("&Remove Node"));
    removeNodeBt->setFocusPolicy(Qt::NoFocus);
    removeNodeBt->setToolTip(
                tr("Remove a node from the network. \n\n "
                   "Alternately, you can remove a node \n"
                   "by right-clicking on it.")
                );

    addEdgeBt= new QPushButton(QIcon(":/images/connect.png"),tr("Add &Edge"));
    addEdgeBt->setFocusPolicy(Qt::NoFocus);
    addEdgeBt->setToolTip(
                tr("Add a new Edge from a node to another (Ctrl+E,Ctrl+A).\n\n "
                   "Alternately, you can create a new edge between two \n"
                   "nodes by middle-clicking on them consecutively.")
                );

    removeEdgeBt= new QPushButton(QIcon(":/images/disconnect.png"),tr("Remove Edge"));
    removeEdgeBt->setFocusPolicy(Qt::NoFocus);
    removeEdgeBt->setToolTip(
                tr("Remove an Edge from the network  \n\n "
                   "Alternately, you can remove an Edge \n"
                   "by right-clicking on it."
                   )
                );

    //create a grid layout for these buttons
    QGridLayout *buttonsGrid = new QGridLayout;
    buttonsGrid -> addWidget(addNodeBt, 0,0);
    buttonsGrid -> addWidget(removeNodeBt, 0,1);
    buttonsGrid -> addWidget(addEdgeBt,1,0);
    buttonsGrid -> addWidget(removeEdgeBt,1,1);
    buttonsGrid->setSpacing(10);
    buttonsGrid->setMargin(0);

    //create a groupbox "Edit" - Inside, display the vertical layout of widgets
    QGroupBox *editGroupBox= new QGroupBox(tr("Edit"));
    editGroupBox->setLayout(buttonsGrid);
    editGroupBox->setMaximumSize(300,100);

    //create widgets for the "Analysis" box
    QLabel *toolBoxAnalysisGeodesicsSelectLabel = new QLabel;
    toolBoxAnalysisGeodesicsSelectLabel->setText(tr("Distances:"));
    toolBoxAnalysisGeodesicsSelect = new QComboBox;
    QStringList geodesicsCommandsList;
    geodesicsCommandsList << "None selected"
                          << "Distance" << "Average Distance"
                          << "Distances Matrix" << "Geodesics Matrix"
                          << "Eccentricity" << "Diameter";
    toolBoxAnalysisGeodesicsSelect->addItems(geodesicsCommandsList);
//    toolBoxAnalysisGeodesicsSelect->setMaximumHeight(20);

    QLabel *toolBoxAnalysisConnectivitySelectLabel  = new QLabel;
    toolBoxAnalysisConnectivitySelectLabel->setText(tr("Connectivity:"));
    toolBoxAnalysisConnectivitySelect = new QComboBox;
    QStringList connectivityCommands;
    connectivityCommands << "None selected"
                         << "Connectedness" << "Walks of given length"
                         << "Total Walks" << "Reachability Matrix";
    toolBoxAnalysisConnectivitySelect->addItems(connectivityCommands);
//    toolBoxAnalysisConnectivitySelect->setMaximumHeight(20);

    QLabel *toolBoxAnalysisClusterabilitySelectLabel  = new QLabel;
    toolBoxAnalysisClusterabilitySelectLabel->setText(tr("Clusterability:"));
    toolBoxAnalysisClusterabilitySelect = new QComboBox;
    QStringList clusterabilityCommands;
    clusterabilityCommands << "None selected"
                         << "Cliques"
                         << "Clustering Coefficient"
                         << "Triad Census";
    toolBoxAnalysisClusterabilitySelect->addItems(clusterabilityCommands);


    QLabel *toolBoxAnalysisProminenceSelectLabel  = new QLabel;
    toolBoxAnalysisProminenceSelectLabel->setText(tr("Prominence:"));
    toolBoxAnalysisProminenceSelect = new QComboBox;
    toolBoxAnalysisProminenceSelect -> setToolTip(
                tr("Various metrics to calculate how 'prominent' or important each actor (node) is inside the network.\n\n")
                );
    toolBoxAnalysisProminenceSelect -> setWhatsThis(
                tr("Various metrics to calculate how 'prominent' or important each actor (node) is inside the network.\n\n") +
                tr("Centrality metrics quantify how central is each node by examining its ties and its geodesic distances (shortest path lengths) to other nodes. ")+
                tr("Most Centrality indices were designed for undirected graphs.\n\n")+
                tr("Prestige indices focus on \"choices received\" to a node. \n")+
                tr("These indices measure the nominations or ties to each node from all others (or inLinks). ") +
                tr("Prestige indices are suitable (and can be calculated only) on directed graphs.")
                );
    QStringList prominenceCommands;
    prominenceCommands << "None selected"
                       << "Degree Centrality" << "Closeness Centrality"
                       << "Influence Range Closeness Centrality"
                       << "Betweenness Centrality"
                       << "Stress Centrality" << "Eccentricity Centrality"
                       << "Power Centrality" << "Information Centrality"
                       << "Degree Prestige (inDegree)"  << "PageRank Prestige"
                       << "Proximity Prestige";
    toolBoxAnalysisProminenceSelect->addItems(prominenceCommands);
//    toolBoxAnalysisProminenceSelect->setMaximumHeight(20);

    //create layout for analysis options
    QGridLayout *analysisGrid = new QGridLayout();
    analysisGrid -> addWidget(toolBoxAnalysisGeodesicsSelectLabel, 0,0);
    analysisGrid -> addWidget(toolBoxAnalysisGeodesicsSelect, 0,1);
    analysisGrid -> addWidget(toolBoxAnalysisConnectivitySelectLabel, 1,0);
    analysisGrid -> addWidget(toolBoxAnalysisConnectivitySelect, 1,1);
    analysisGrid -> addWidget(toolBoxAnalysisClusterabilitySelectLabel, 3,0);
    analysisGrid -> addWidget(toolBoxAnalysisClusterabilitySelect, 3,1);
    analysisGrid -> addWidget(toolBoxAnalysisProminenceSelectLabel, 4,0);
    analysisGrid -> addWidget(toolBoxAnalysisProminenceSelect, 4,1);
    //layoutByIndexGrid -> setRowStretch(0,1);   //fix stretch

    //create a box and set the above layout inside
    QGroupBox *analysisBox= new QGroupBox(tr("Analyze"));
    analysisBox->setMaximumWidth(300);
    analysisBox->setLayout (analysisGrid );


    //create widgets for the "Visualization By Index" box
    QLabel *toolBoxLayoutByIndexSelectLabel = new QLabel;
    toolBoxLayoutByIndexSelectLabel->setText(tr("Index:"));
    toolBoxLayoutByIndexSelect = new QComboBox;
    QStringList indicesList;
    indicesList << "None/Original"<< "Random"
                << "Degree Centrality" << "Closeness Centrality"
                << "Influence Range Closeness Centrality"
                << "Betweenness Centrality"
                << "Stress Centrality" << "Eccentricity Centrality"
                << "Power Centrality" << "Information Centrality"
                << "Degree Prestige (inDegree)"  << "PageRank Prestige"
                << "Proximity Prestige";
    toolBoxLayoutByIndexSelect->addItems(indicesList);
    toolBoxLayoutByIndexSelect->setMinimumHeight(20);


    QLabel *toolBoxLayoutByIndexTypeLabel = new QLabel;
    toolBoxLayoutByIndexTypeLabel->setText(tr("Layout Type:"));
    toolBoxLayoutByIndexTypeSelect = new QComboBox;
    QStringList layoutTypes;
    layoutTypes << "Circular" << "On Levels" << "Nodal size";
    toolBoxLayoutByIndexTypeSelect->addItems(layoutTypes);
    toolBoxLayoutByIndexTypeSelect->setMinimumHeight(20);

    toolBoxLayoutByIndexButton = new QPushButton(tr("Apply"));
    toolBoxLayoutByIndexButton->setFocusPolicy(Qt::NoFocus);
    toolBoxLayoutByIndexButton->setMinimumHeight(20);
    toolBoxLayoutByIndexButton->setMaximumWidth(60);


    //create layout for visualisation by index options
    QGridLayout *layoutByIndexGrid = new QGridLayout();
    layoutByIndexGrid -> addWidget(toolBoxLayoutByIndexSelectLabel, 0,0);
    layoutByIndexGrid -> addWidget(toolBoxLayoutByIndexSelect, 0,1);
    layoutByIndexGrid -> addWidget(toolBoxLayoutByIndexTypeLabel, 1,0);
    layoutByIndexGrid -> addWidget(toolBoxLayoutByIndexTypeSelect, 1,1);
    layoutByIndexGrid -> addWidget(toolBoxLayoutByIndexButton, 2,1);
    //layoutByIndexGrid -> setRowStretch(0,1);   //fix stretch

    //create a box and set the above layout inside
    QGroupBox *layoutByIndexBox= new QGroupBox(tr("By Prominence Index"));
    layoutByIndexBox->setMaximumWidth(300);
    layoutByIndexBox->setLayout (layoutByIndexGrid );


    // create widgets for the "Force-Directed Models" groupBox
    layoutEadesBx = new QCheckBox(tr("Spring Embedder (Eades)") );
    layoutEadesBx->setEnabled(true);
    layoutEadesBx->setChecked(false);
    layoutEadesBx
            ->setToolTip(
                tr("Embeds a spring-gravitational model on the network, where \n"
                   "each node is regarded as physical object (ring) repeling all \n"
                   "other nodes, while springs between connected nodes attract them. \n"
                   "This is a very SLOW process on networks with N > 100!"));

    layoutFruchtermanBx = new QCheckBox(tr("Fruchterman-Reingold") );
    layoutFruchtermanBx->setEnabled(true);
    layoutFruchtermanBx->setChecked(false);
    layoutFruchtermanBx
            ->setToolTip(
                tr("In Fruchterman-Reingold model, the vertices behave as atomic \n"
                   "particles or celestial bodies, exerting attractive and repulsive \n"
                   "forces to each other. Again, only vertices that are neighbours \n"
                   "attract each other but, unlike Eades Spring Embedder, all vertices \n"
                   "repel each other. "));

    layoutKamandaBx= new QCheckBox(tr("Kamanda-Kawai") );
    layoutKamandaBx->setEnabled(false);
    layoutKamandaBx->setToolTip(tr("!"));

    //create layout for dynamic visualisation
    QGridLayout *layoutForceDirectedGrid = new QGridLayout();
    layoutForceDirectedGrid -> addWidget(layoutEadesBx, 0,0);
    layoutForceDirectedGrid -> addWidget(layoutFruchtermanBx, 1,0);
    layoutForceDirectedGrid -> addWidget(layoutKamandaBx, 2,0);
    layoutForceDirectedGrid->setSpacing(10);
    layoutForceDirectedGrid->setMargin(0);

    //create a box for dynamic layout options
    QGroupBox *layoutDynamicBox= new QGroupBox(tr("By Force-Directed Model"));
    layoutDynamicBox->setMaximumWidth(300);
    layoutDynamicBox->setLayout (layoutForceDirectedGrid );


    //create widgets for additional visualization options box
    nodeSizesByOutDegreeBx = new QCheckBox(
                tr("Node sizes by OutDegree") );
    nodeSizesByOutDegreeBx ->setEnabled(true);
    nodeSizesByOutDegreeBx
            ->setToolTip(
                tr("If you enable this, all nodes will be resized so that their "
                   "size reflect their out-degree. \n"
                   "Nodes with more directed edges starting from them will be bigger..."));

    nodeSizesByInDegreeBx = new QCheckBox(
                tr("Node sizes by InDegree") );
    nodeSizesByInDegreeBx ->setEnabled(true);
    nodeSizesByInDegreeBx
            ->setToolTip(
                tr("If you enable this, all nodes will be resized so that their "
                   "size reflect their in-degree. \n"
                   "Nodes with more directed edges ending at them will be bigger..."));

    layoutGuidesBx = new QCheckBox(
                tr("Layout guidelines") );
    layoutGuidesBx ->setEnabled(true);
    layoutGuidesBx ->setChecked(true);
    layoutGuidesBx->setToolTip(
                tr("Disable to not display layout guidelines"));

    QGridLayout *layoutOptionsGrid = new QGridLayout();
    layoutOptionsGrid -> addWidget(nodeSizesByOutDegreeBx, 0,0);
    layoutOptionsGrid -> addWidget(nodeSizesByInDegreeBx, 1,0);
    layoutOptionsGrid -> addWidget(layoutGuidesBx, 2,0);
    layoutOptionsGrid->setSpacing(10);
    layoutOptionsGrid->setMargin(0);

    //Box for additional layout options
    QGroupBox *layoutOptionsBox= new QGroupBox(tr("Options"));
    layoutOptionsBox->setMaximumWidth(300);
    layoutOptionsBox->setLayout (layoutOptionsGrid );


    //Parent box with vertical layout for all layout/visualization boxes
    QVBoxLayout *visualizationBoxLayout = new QVBoxLayout;
    visualizationBoxLayout -> addWidget(layoutByIndexBox);
    visualizationBoxLayout -> addWidget(layoutDynamicBox);
    visualizationBoxLayout -> addWidget(layoutOptionsBox);
    QGroupBox *visualizationBox= new QGroupBox(tr("Visualize"));
    visualizationBox->setMaximumWidth(300);
    visualizationBox->setLayout (visualizationBoxLayout );

    //Parent box with vertical layout for all boxes of Controls
    QVBoxLayout *controlTabVerticalLayout = new QVBoxLayout;
    controlTabVerticalLayout -> addWidget(editGroupBox);
    controlTabVerticalLayout -> addWidget(analysisBox);
    controlTabVerticalLayout -> addWidget(visualizationBox);
    controlTabVerticalLayout->setSpacing(0);
    controlTabVerticalLayout->setMargin(0);

    QGroupBox *controlGroupBox = new QGroupBox;
    controlGroupBox->setLayout(controlTabVerticalLayout);
    controlGroupBox->setMaximumWidth(300);
    controlGroupBox->setContentsMargins(0,0,0,0);
    toolBox->addTab(controlGroupBox, tr("Controls"));


    connect(layoutEadesBx, SIGNAL(clicked(bool)),
            this, SLOT(slotLayoutSpringEmbedder(bool)));
    connect(layoutFruchtermanBx, SIGNAL(stateChanged(int)),
            this, SLOT(layoutFruchterman(int)));

    connect(nodeSizesByOutDegreeBx , SIGNAL(clicked(bool)),
            this, SLOT(slotLayoutNodeSizesByOutDegree(bool)));
    connect(nodeSizesByInDegreeBx , SIGNAL(clicked(bool)),
            this, SLOT(slotLayoutNodeSizesByInDegree(bool)));


    //create widgets for Properties/Statistics group/tab
    QLabel *labelNodesLCD = new QLabel;
    labelNodesLCD->setText(tr("Total Nodes"));
    QLabel *labelEdgesLCD = new QLabel;
    labelEdgesLCD->setText(tr("Total Edges (Arcs)"));
    nodesLCD=new QLCDNumber(7);
    nodesLCD->setSegmentStyle(QLCDNumber::Flat);
    nodesLCD->setToolTip(tr("Counts how many nodes (vertices) exist in the whole network."));
    edgesLCD=new QLCDNumber(7);
    edgesLCD->setSegmentStyle(QLCDNumber::Flat);
    edgesLCD->setToolTip(tr("Counts how many edges (arcs) exist in the whole network."));

    QLabel *labelDensityLCD = new QLabel;
    labelDensityLCD->setText(tr("Density"));
    densityLCD=new QLCDNumber(7);
    densityLCD->setSegmentStyle(QLCDNumber::Flat);
    densityLCD->setToolTip(tr("The density of a network is the ratio of existing edges to all possible edges ( n*(n-1) ) between nodes."));

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
    labelNode ->setFont(QFont("sans-serif", 10, QFont::Bold));

    QLabel *labelSelectedNodeLCD = new QLabel;
    labelSelectedNodeLCD -> setText (tr("Node Number:"));
    labelSelectedNodeLCD -> setToolTip (tr("This is the number of the last selected node."));

    selectedNodeLCD =new QLCDNumber(7);
    selectedNodeLCD ->setSegmentStyle(QLCDNumber::Flat);

    QLabel *labelInDegreeLCD = new QLabel;
    labelInDegreeLCD -> setText (tr("Node In-Degree:"));
    labelInDegreeLCD -> setToolTip (tr("The sum of all in-edge weights of the node you clicked.."));
    inDegreeLCD=new QLCDNumber(7);
    inDegreeLCD -> setSegmentStyle(QLCDNumber::Flat);
    inDegreeLCD -> setToolTip (tr("The sum of all in-edge weights of the node you clicked."));
    QLabel *labelOutDegreeLCD = new QLabel;
    labelOutDegreeLCD -> setText (tr("Node Out-Degree:"));
    labelOutDegreeLCD -> setToolTip (tr("The sum of all out-edge weights of the node you clicked."));
    outDegreeLCD=new QLCDNumber(7);
    outDegreeLCD -> setSegmentStyle(QLCDNumber::Flat);
    outDegreeLCD -> setToolTip (tr("The sum of all out-edge weights of the node you clicked."));

    QLabel *labelClucofLCD  = new QLabel;
    labelClucofLCD -> setText (tr("Clustering Coef."));
    labelClucofLCD -> setToolTip (tr("The Clustering Coefficient quantifies how close the clicked vertex and its neighbors are to being a clique. \nThe value is the proportion of Edges between the vertices within the neighbourhood of the clicked vertex,\n divided by the number of Edges that could possibly exist between them. \n\n WARNING: This value is automatically calculated only if vertices < 500.\n If your network is larger than 500 vertices, compute CluCof from the menu Analysis > Clustering Coefficient "));
    clucofLCD = new QLCDNumber(7);
    clucofLCD -> setSegmentStyle(QLCDNumber::Flat);
    clucofLCD  -> setToolTip (tr("The Clustering Coefficient quantifies how close the clicked vertex and its neighbors are to being a clique. \nThe value is the proportion of Edges between the vertices within the neighbourhood of the clicked vertex,\n divided by the number of Edges that could possibly exist between them. \n\n This value is automatically calculated only if vertices < 500.\n If your network is larger than 500 vertices, compute CluCof from the menu Analysis > Clustering Coefficient "));


    propertiesGrid -> addWidget(dummyLabel, 6,0);
    propertiesGrid -> addWidget(labelNode, 7,0);
    propertiesGrid -> addWidget(labelSelectedNodeLCD , 8,0);
    propertiesGrid -> addWidget(selectedNodeLCD ,8,1);
    propertiesGrid -> addWidget(labelInDegreeLCD, 9,0);
    propertiesGrid -> addWidget(inDegreeLCD, 9,1);
    propertiesGrid -> addWidget(labelOutDegreeLCD, 10,0);
    propertiesGrid -> addWidget(outDegreeLCD,10,1);
    propertiesGrid -> addWidget(labelClucofLCD, 11,0);
    propertiesGrid -> addWidget(clucofLCD,11,1);
    propertiesGrid -> setRowStretch(12,1);   //fix stretch

    //create a box with title
    QGroupBox *networkPropertiesGroup = new QGroupBox(tr(""));
    networkPropertiesGroup -> setLayout (propertiesGrid);


    toolBox->addTab( networkPropertiesGroup, tr("Statistics"));
    toolBox->setMinimumWidth(controlGroupBox->sizeHint().width());
    toolBox->setFixedWidth(300);
    //toolBox->setStyleSheet("* { background-color:#ededed}}");

}


//Called from MW, when user selects something in the Geodesics selectbox
// of toolbox
void MainWindow::toolBoxAnalysisGeodesicsSelectChanged(int selectedIndex) {
    qDebug()<< "MW::toolBoxAnalysisGeodesicsSelectChanged "
               "selected text index: " << selectedIndex;
    switch(selectedIndex){
    case 0:
        break;
    case 1:
        slotGraphDistance();
        break;
    case 2:
        slotAverageGraphDistance();
        break;
    case 3:
        slotDistancesMatrix();
        break;
    case 4:
        slotGeodesicsMatrix();
        break;
    case 5:
        slotEccentricity();
        break;
    case 6:
        slotDiameter();
        break;
    };


}




//Called from MW, when user selects something in the Connectivity selectbox
// of toolbox
void MainWindow::toolBoxAnalysisConnectivitySelectChanged(int selectedIndex) {
    qDebug()<< "MW::toolBoxAnalysisConnectivitySelectChanged "
               "selected text index: " << selectedIndex;
    switch(selectedIndex){
    case 0:
        break;
    case 1:
        qDebug()<< "Connectedness";
        slotConnectedness();
        break;
    case 2:
        qDebug()<< "Walks of given length";
        slotWalksOfGivenLength();
        break;
    case 3:
        qDebug() << "Total Walks selected";
        slotTotalWalks();
        break;
    case 4:
        qDebug() << "Reachability Matrix";
        slotReachabilityMatrix();
        break;
    };

}



//Called from MW, when user selects something in the Clusterability selectbox
// of toolbox
void MainWindow::toolBoxAnalysisClusterabilitySelectChanged(int selectedIndex) {
    qDebug()<< "MW::toolBoxAnalysisClusterabilitySelectChanged "
               "selected text index: " << selectedIndex;
    switch(selectedIndex){
    case 0:
        break;
    case 1:
        qDebug()<< "Cliques";
        slotCliqueCensus();
        break;
    case 2:
        qDebug()<< "Clustering Coefficient";
        slotClusteringCoefficient();
        break;
    case 3:
        qDebug() << "Triad Census";
        slotTriadCensus();
        break;

    };

}




//Called from MW, when user selects something in the Prominence selectbox
// of toolbox
void MainWindow::toolBoxAnalysisProminenceSelectChanged(int selectedIndex) {
    qDebug()<< "MW::toolBoxAnalysisProminenceSelectChanged "
               "selected text index: " << selectedIndex;
    switch(selectedIndex){
    case 0:
        break;
    case 1:
        slotCentralityDegree();
        break;
    case 2:
        slotCentralityCloseness();
        break;
    case 3:
        slotCentralityClosenessInfluenceRange();
        break;
    case 4:
        slotCentralityBetweenness();
        break;
    case 5:
        slotCentralityStress();
        break;
    case 6:
        slotCentralityEccentricity();
        break;
    case 7:
        slotCentralityPower();
        break;
    case 8:
        slotCentralityInformation();
        break;
    case 9:
        slotPrestigeDegree();
        break;
    case 10:
        slotPrestigePageRank();
        break;
    case 11:
        slotPrestigeProximity();
        break;
    };


}

void MainWindow::toolBoxLayoutByIndexButtonPressed(){
    qDebug()<<"MW::toolBoxLayoutByIndexButtonPressed()";
    int selectedIndex = toolBoxLayoutByIndexSelect->currentIndex();
    QString selectedIndexText = toolBoxLayoutByIndexSelect -> currentText();
    int selectedLayoutType = toolBoxLayoutByIndexTypeSelect ->currentIndex();
    qDebug() << " selected index is " << selectedIndexText << " : " << selectedIndex
             << " selected layout type is " << selectedLayoutType;
    switch(selectedIndex) {
    case 0:
        break;
    case 1:
        if (selectedLayoutType==0)
            slotLayoutCircularRandom();
        else if (selectedLayoutType==1)
            slotLayoutRandom();
        break;
    default:
        if (selectedLayoutType==0)
            slotLayoutCircularByProminenceIndex(selectedIndexText);
        else if (selectedLayoutType==1)
            slotLayoutLevelByProminenceIndex(selectedIndexText);
        else if (selectedLayoutType==2){
            slotLayoutNodeSizesByProminenceIndex(selectedIndexText);
            // re-init other options for node sizes...
            nodeSizesByOutDegreeAct->setChecked(false);
            nodeSizesByOutDegreeBx->setChecked(false);
            nodeSizesByInDegreeAct->setChecked(false);
            nodeSizesByInDegreeBx->setChecked(false);
        }
        break;
    };
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

    this->resize(1024,768);

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

    initNodeSize=8;
    initNodeColor="red";
    initEdgeColor="black";
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
    networkName="";

    previous_fileName=fileName;
    fileName="";

    pajekFileLoaded=false;
    adjacencyFileLoaded=false;
    fileFormat = -1;
    initFileCodec= "UTF-8";
    checkSelectFileType = true;
    dotFileLoaded=false;
    fileLoaded=false;

    networkModified=false;
    fileSave->setIcon(QIcon(":/images/saved.png"));
    fileSave->setEnabled(true);

    markedNodesExist=false;	//used by slotFindNode()

    cursorPosGW=QPointF(-1,-1);
    clickedJimNumber=-1;
    edgeClicked=false;
    nodeClicked=false;

    considerWeights=false;
    inverseWeights=false;
    askedAboutWeights=false;

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

    activeGraph.setInitEdgeColor(initEdgeColor);

    activeGraph.setShowLabels(this->showLabels());
    activeGraph.setShowNumbersInsideNodes( this->showNumbersInsideNodes());

    /** Clear scene **/
    graphicsWidget->clear();

    /** Clear LCDs **/
    nodesLCD->display(activeGraph.vertices());
    edgesLCD->display(activeEdges());
    densityLCD->display(activeGraph.density());
    inDegreeLCD->display(0);
    outDegreeLCD->display(0);
    clucofLCD->display(0);
    selectedNodeLCD->display(0);

    /** Clear toolbox and menu checkboxes **/
    toolBoxAnalysisClusterabilitySelect->setCurrentIndex(0);
    toolBoxAnalysisConnectivitySelect->setCurrentIndex(0);
    toolBoxAnalysisGeodesicsSelect->setCurrentIndex(0);
    toolBoxAnalysisProminenceSelect->setCurrentIndex(0);
    toolBoxLayoutByIndexSelect->setCurrentIndex(0);
    toolBoxLayoutByIndexTypeSelect ->setCurrentIndex(0);
    nodeSizesByOutDegreeBx->setChecked(false);
    nodeSizesByInDegreeBx->setChecked(false);
    layoutEadesBx->setChecked(false);
    springLayoutAct->setChecked(false);
    FRLayoutAct->setChecked(false);
    displayEdgesWeightNumbersAct->setChecked(false);
    considerEdgeWeightsAct->setChecked(false);
    //displayEdgesArrowsAct->setChecked(false);		//FIXME: USER PREFS EMITTED TO GRAPH?

    filterIsolateNodesAct->setChecked(false); // re-init orphan nodes menu item

    changeRelationCombo->clear();

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

void MainWindow::showMessageToUser(const QString message) {
    QMessageBox::information(this, tr("Info"),
                          message,
                          QMessageBox::Ok, 0);
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
 * @brief MainWindow::getLastPath
 * returns the last path used by user to open/save something
 */
QString MainWindow::getLastPath() {
    if ( lastUsedDirPath == "socnetv-initial-none") {
        lastUsedDirPath = QDir::homePath();
    }
    qDebug() << lastUsedDirPath ;
    return lastUsedDirPath ;
}


/**
 * @brief MainWindow::setLastPath
 * sets the last path used by user to open/save something
 * @param filePath
 */
void MainWindow::setLastPath(QString filePath) {
    lastUsedDirPath = filePath.left( filePath.lastIndexOf("/"));
    qDebug() << lastUsedDirPath;
}


/**
    Prompts the user a directory dialogue to choose a file from.
    Calls previewNetworkFile()
*/
void MainWindow::slotChooseFile() {

    if (firstTime && fileFormat == -500 ) {
        QMessageBox::information( this, "SocNetV",
                                  tr("Attention: \n")+
                                  tr("This menu option is more suitable for loading "
                                     "a network file in GraphML format (.graphml), "
                                     "which is the default format of SocNetV. \n"
                                     "Nevertheless, if you select other supported "
                                     "filetype SocNetV will attempt to load it.\n")+

                                  tr("If your file is not GraphML but you know its "
                                     "format is supported (i.e. Pajek, UCINET, GraphViz, etc), ")+
                                  tr("please use the options in the Import sub menu. They are more safe.\n")+
                                  tr("\n This warning message will not appear again."),
                                  "OK", 0 );
        firstTime=false;
    }
    if ( fileFormat == -1 )
        fileFormat = -1;

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
        fileType_string = tr("Adjacency (*.csv *.sm *.adj);;All (*)");
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
        fileType_string = tr("Weighted List (*.wlst *.wlist);;All (*)");
        break;
    case 8:	// Simple List
        fileType_string = tr("List (*.lst *.list);;All (*)");
        break;
    case 9:	// Two mode sm
        fileType_string = tr("Two-Mode Sociomatrix (*.2sm *.aff);;All (*)");
        break;
    default:	//All
        fileType_string = tr("GraphML (*.graphml *.xml);;Pajek (*.net *.pajek *.paj);;DL (*.dl *.dat);;Adjacency (*.csv *.adj *.sm);;GraphViz (*.dot);;List (*.lst *.list);;Weighted List (*.wlst *.wlist);;All (*)");
        break;

    }

    m_fileName = QFileDialog::getOpenFileName(
                this,
                tr("Select one file to open"),
                getLastPath(), fileType_string	);

    if (checkSelectFileType) {
        //check if user has changed the filetype filter and loaded other filetype
        if (m_fileName.endsWith(".graphml",Qt::CaseInsensitive ) ||
                m_fileName.endsWith(".xml",Qt::CaseInsensitive ) ) {
            fileFormat=m_fileFormat=1;
        }
        else if (m_fileName.endsWith(".net",Qt::CaseInsensitive ) ||
                 m_fileName.endsWith(".paj",Qt::CaseInsensitive )  ||
                 m_fileName.endsWith(".pajek",Qt::CaseInsensitive ) ) {
            fileFormat=m_fileFormat=2;
        }
        else if (m_fileName.endsWith(".sm",Qt::CaseInsensitive ) ||
                 m_fileName.endsWith(".dat",Qt::CaseInsensitive )  ||
                 m_fileName.endsWith(".adj",Qt::CaseInsensitive ) ||
                 m_fileName.endsWith(".txt",Qt::CaseInsensitive )) {
            fileFormat=m_fileFormat=3;
        }
        else if (m_fileName.endsWith(".dot",Qt::CaseInsensitive ) ) {
            fileFormat=m_fileFormat=4;
        }
        else if (m_fileName.endsWith(".gml",Qt::CaseInsensitive ) ) {
            fileFormat=m_fileFormat=5;
        }
        else if (m_fileName.endsWith(".dl",Qt::CaseInsensitive ) ) {
            fileFormat=m_fileFormat=6;
        }
        else if (m_fileName.endsWith(".list",Qt::CaseInsensitive ) ||
                 m_fileName.endsWith(".lst",Qt::CaseInsensitive )  ) {
            fileFormat=m_fileFormat=7;
        }
        else if (m_fileName.endsWith(".wlist",Qt::CaseInsensitive ) ||
                 m_fileName.endsWith(".wlst",Qt::CaseInsensitive )  ) {
            fileFormat=m_fileFormat=8;
        }
        else if (m_fileName.endsWith(".2sm",Qt::CaseInsensitive ) ||
                 m_fileName.endsWith(".aff",Qt::CaseInsensitive )  ) {
            fileFormat=m_fileFormat=9;
        }
        else
            fileFormat=m_fileFormat=-1;
    }
    if (!m_fileName.isEmpty()) {
        if (m_fileFormat == -1) {
            QMessageBox::critical(this, "Unrecognized file extension", tr("Error! \n"
                                  "SocNetV supports the following network file"
                                  "formats. The filename you selected does not "
                                  "end with any of the following extensions:\n"
                                  "- GraphML (.graphml or .xml)\n"
                                  "- Pajek (.paj or .pajek or .net)\n"
                                  "- UCINET (.dl) \n"
                                  "- GraphViz (.dot)\n"
                                  "- Adjacency Matrix (.sm or .adj or .csv)\n"
                                  "- List (.list or .lst)\n"
                                  "- Weighted List (.wlist or .wlst)\n"
                                  "- Two-Mode / affiliation (.2sm or .aff) \n\n"
                                  "If you are sure the file is of a supported "
                                  "format, perhaps you should just change its extension..."),
                                  QMessageBox::Ok, 0);
        }
        qDebug()<<"MW: file selected: " << m_fileName;
        fileNameNoPath=m_fileName.split ("/");
        setLastPath(m_fileName); // store this path

        previewNetworkFile(m_fileName, m_fileFormat );

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

    QString fn =  QFileDialog::getSaveFileName(
                this,
                tr("Save GraphML Network to File Named..."),
                getLastPath(), tr("GraphML (*.graphml *.xml);;All (*)") );
    if (!fn.isEmpty())  {
        if  ( QFileInfo(fn).suffix().isEmpty() ){
            QMessageBox::information(this, "Missing Extension ",tr("File extension was missing! \nI am appending a standard .graphml to the given filename."), "OK",0);
            fn.append(".graphml");
        }
        fileName=fn;
        fileNameNoPath=fileName.split ("/");
        setLastPath(fileName); // store this path
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
//    fileFormat=-1;
//    checkSelectFileType = true;
    this->slotChooseFile();
}



/**	
    Imports a network from a formatted file
*/
void MainWindow::slotImportPajek(){
    fileFormat=2;
    checkSelectFileType = false;
    this->slotChooseFile();
}



/**	
    Imports a network from a Adjacency matrix formatted file
*/
void MainWindow::slotImportSM(){
    fileFormat=3;
    checkSelectFileType = false;
    this->slotChooseFile();
}



/**
    Imports a network from a two mode sociomatrix formatted file
*/
void MainWindow::slotImportTwoModeSM(){
    fileFormat=9;
    checkSelectFileType = false;
    this->slotChooseFile();
}


/**	
    Imports a network from a Dot formatted file
*/
void MainWindow::slotImportDot(){
    fileFormat=4;
    checkSelectFileType = false;
    this->slotChooseFile();
}



/**	
    Imports a network from a GML formatted file
*/
void MainWindow::slotImportGML(){
    fileFormat=5;
    checkSelectFileType = false;
    this->slotChooseFile();
}


/**	
    Imports a network from a UCINET formatted file
*/
void MainWindow::slotImportDL(){
    fileFormat=6;
    checkSelectFileType = false;
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
    checkSelectFileType = false;
    this->slotChooseFile();
}


void MainWindow::findCodecs()
{
    QMap<QString, QTextCodec *> codecMap;
    QRegExp iso8859RegExp("ISO[- ]8859-([0-9]+).*");

    foreach (int mib, QTextCodec::availableMibs()) {
        QTextCodec *codec = QTextCodec::codecForMib(mib);

        QString sortKey = codec->name().toUpper();
        int rank;

        if (sortKey.startsWith("UTF-8")) {
            rank = 1;
        } else if (sortKey.startsWith("UTF-16")) {
            rank = 2;
        } else if (iso8859RegExp.exactMatch(sortKey)) {
            if (iso8859RegExp.cap(1).size() == 1)
                rank = 3;
            else
                rank = 4;
        } else {
            rank = 5;
        }
        sortKey.prepend(QChar('0' + rank));

        codecMap.insert(sortKey, codec);
    }
    codecs = codecMap.values();
}



bool MainWindow::previewNetworkFile(QString m_fileName, int m_fileFormat ){
    qDebug() << "MW::previewNetworkFile() : "<< m_fileName;

    if (!m_fileName.isEmpty()) {
        QFile file(m_fileName);
        if (!file.open(QFile::ReadOnly)) {
            QMessageBox::warning(this, tr("previewNetworkFile"),
                                 tr("Cannot read file %1:\n%2")
                                 .arg(m_fileName)
                                 .arg(file.errorString()));
            return false;
        }
        qDebug() << "MW::loadNetworkFile() reading the file now... " ;
        QByteArray data = file.readAll();

        previewForm->setEncodedData(data,m_fileName, m_fileFormat);
        previewForm->exec();
    }
    return true;
}

void MainWindow::userCodec(const QString m_fileName,
                           const QString m_codecName,
                           const int m_fileFormat) {
    loadNetworkFile(m_fileName, m_codecName, m_fileFormat );
}


/**
 * @brief MainWindow::loadNetworkFile
 * Main network file loader method
 * First, inits everything to default values.
 * Then calls activeGraph::loadGraph to actually load the network...
 * @param m_fileName
 * @param m_fileFormat
 * @return
 */
bool MainWindow::loadNetworkFile(const QString m_fileName,
                                 const QString m_codecName,
                                 const int m_fileFormat )
{
    qDebug() << "MW::loadNetworkFile() : "<< m_fileName
                    << " m_codecName " << m_codecName
                    << " m_fileFormat " << m_fileFormat;
    initNet();

    userSelectedCodecName = m_codecName; //var for future use in a Settings dialog

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
    qDebug() << "MW::loadNetworkFile() : calling activeGraph.loadGraph() ";
    bool loadGraphStatus = activeGraph.loadGraph (
                m_fileName,
                m_codecName,
                displayNodeLabelsAct->isChecked(),
                graphicsWidget->width(),
                graphicsWidget->height(),
                m_fileFormat, two_sm_mode
                );
    qDebug() << "MW::loadNetworkFile() : loadGraphStatus " << loadGraphStatus;
    if ( loadGraphStatus )
    {
        fileName=m_fileName;
        previous_fileName=fileName;
        fileNameNoPath = fileName.split("/");
        Q_ASSERT_X( !fileNameNoPath.isEmpty(),  "not empty filename ", "empty filename " );
        setWindowTitle("SocNetV "+ VERSION +" - "+fileNameNoPath.last());
        QString message=tr("Loaded network: ")+fileNameNoPath.last();
        statusMessage( message );
    }
    else {
        statusMessage( tr("Error loading requested file. Aborted."));
        QMessageBox::critical( this, "SocNetV",
                               tr("Error! \n")+
                               tr("Sorry, the selected file is not in valid format or encoding. \n")+
                               tr("Try a different codec in the preview window or if you are trying to import legacy formats (i.e. Pajek, UCINET, dot, etc), ")+
                               tr("please use the options in the Import sub menu. \n"),
                               "OK", 0 );
    }
    QApplication::restoreOverrideCursor();
    qDebug() << "MW::loadNetworkFile() : returning " << loadGraphStatus;
    return loadGraphStatus;
}


/**
 * @brief MainWindow::fileType
 * Called from Parser/Graph when a network file is loaded.
 * It informs the MW about the type of the network so that it can display the appropiate message.
 * @param type
 * @param netName
 * @param aNodes
 * @param totalEdges
 * @param undirected
 */
void MainWindow::fileType (
        int type, QString netName, int aNodes, int totalEdges, bool undirected)
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
        statusMessage( QString(tr("GraphML formatted network, named %1, loaded with %2 Nodes and %3 total Edges.")).arg( networkName ).arg( aNodes ).arg(totalEdges ) );
        break;

    case 2:
        pajekFileLoaded=true;
        adjacencyFileLoaded=false;
        graphMLFileLoaded=false;
        fileLoaded=true;
        networkModified=false;
        statusMessage( QString(tr("Pajek formatted network, named %1, loaded with %2 Nodes and %3 total Edges.")).arg( networkName ).arg( aNodes ).arg(totalEdges ));
        break;

    case 3:
        pajekFileLoaded=false;
        adjacencyFileLoaded=true;
        graphMLFileLoaded=false;
        fileLoaded=true;
        networkModified=false;
        statusMessage( QString(tr("Adjacency formatted network, named %1, loaded with %2 Nodes and %3 total Edges.")).arg( networkName ).arg( aNodes ).arg(totalEdges ) );
        break;

    case 4:
        pajekFileLoaded=false;
        adjacencyFileLoaded=false;
        dotFileLoaded=true;
        graphMLFileLoaded=false;
        fileLoaded=true;
        networkModified=false;
        statusMessage( QString(tr("Dot formatted network, named %1, loaded with %2 Nodes and %3 total Edges.")).arg( networkName ).arg( aNodes ).arg(totalEdges ) );
        break;

    case 5:
        pajekFileLoaded=false;
        adjacencyFileLoaded=false;
        dotFileLoaded=false;
        graphMLFileLoaded=false;
        fileLoaded=true;
        networkModified=false;
        statusMessage( QString(tr("DL-formatted network, named %1, loaded with %2 Nodes and %3 total Edges.")).arg( networkName ).arg( aNodes ).arg(totalEdges ) );
        break;
    case 6:
        pajekFileLoaded=false;
        adjacencyFileLoaded=false;
        dotFileLoaded=false;
        graphMLFileLoaded=false;
        fileLoaded=true;
        networkModified=false;
        statusMessage( QString(tr("GML-formatted network, named %1, loaded with %2 Nodes and %3 total Edges.")).arg( networkName ).arg( aNodes ).arg(totalEdges ) );
        break;
    case 7:
        pajekFileLoaded=false;
        adjacencyFileLoaded=false;
        dotFileLoaded=false;
        graphMLFileLoaded=false;
        fileLoaded=true;
        networkModified=false;
        statusMessage( QString(tr("Weighted list-formatted network, named %1, loaded with %2 Nodes and %3 total Edges.")).arg( networkName ).arg( aNodes ).arg(totalEdges ) );
        break;
    case 8:
        pajekFileLoaded=false;
        adjacencyFileLoaded=false;
        dotFileLoaded=false;
        graphMLFileLoaded=false;
        fileLoaded=true;
        networkModified=false;
        statusMessage( QString(tr("Simple list-formatted network, named %1, loaded with %2 Nodes and %3 total Edges.")).arg( networkName ).arg( aNodes ).arg(totalEdges ) );
        break;
    case 9:
        pajekFileLoaded=false;
        adjacencyFileLoaded=false;
        dotFileLoaded=false;
        graphMLFileLoaded=false;
        fileLoaded=true;
        networkModified=false;
        statusMessage( QString(tr("Two-mode affiliation network, named %1, loaded with %2 Nodes and %3 total Edges.")).arg( networkName ).arg( aNodes ).arg(totalEdges ) );
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
 * @brief MainWindow::prevRelation
 * Decreases the index of changeRelationCombo
 * which signals to Graph::changeRelation()
 */
void MainWindow::prevRelation(){
    qDebug() << "MW::prevRelation()";
    int index=changeRelationCombo->currentIndex();
    if (index>0){
        --index;
        filterIsolateNodesAct->setChecked(false);
        changeRelationCombo->setCurrentIndex(index);
    }
}

/**
 * @brief MainWindow::nextRelation
 * Increases the index of changeRelationCombo
 * which signals to Graph::changeRelation()
 */
void MainWindow::nextRelation(){
    qDebug() << "MW::nextRelation()";
    int index=changeRelationCombo->currentIndex();
    int relationsCounter=changeRelationCombo->count();
    if (index< (relationsCounter -1 )){
        ++index;
        filterIsolateNodesAct->setChecked(false);
        changeRelationCombo->setCurrentIndex(index);
    }

}



/**
 * @brief MainWindow::addRelation
 * called from activeGraph::addRelationFromGraph(QString) when the parser or a
 * Graph method demands a new relation to be added in the Combobox.
 * @param relationName (NULL)
 */
void MainWindow::addRelation(QString relationName){
    qDebug() << "MW::addRelation(string)" << relationName;
    if ( !relationName.isNull() ){
        changeRelationCombo->addItem(relationName);
    }
}

/**
 * @brief MainWindow::addRelation
 * Called from MW when user clicks New Relation btn
 * or when the user creates the first edge visually.
 */
void MainWindow::addRelation(){
    qDebug() << "MW::addRelation()";
    bool ok;
    QString newRelationName;
    int relationsCounter=changeRelationCombo->count();
    if (relationsCounter==0) {
        newRelationName = QInputDialog::getText(this, tr("Add new relation"),
                              tr("Since you have just created the first edge "
                                 "of this social network, please enter a name \n"
                                 "for this new relation between the actors.\n "
                                 "A relation is a collection of ties of a "
                                 "specific kind between the network actors.\n"
                                 "For instance, enter \"friendship\" if the "
                                 "edges of this relation refer to the set of \n"
                                 "friendships between pairs of actors."),
                              QLineEdit::Normal, QString::null, &ok );
    }
    else {
        newRelationName = QInputDialog::getText(
                    this, tr("Add new relation"),
                    tr("Please enter a name for the new relation:"),
                    QLineEdit::Normal,QString::null, &ok );
    }
    if (ok && !newRelationName.isEmpty()){
        changeRelationCombo->addItem(newRelationName);
        emit addRelationToGraph(newRelationName);
        if (relationsCounter != 0){ //dont do it if its the first relation added
            qDebug() << "MW::addRelation() - updating combo index";
            changeRelationCombo->setCurrentIndex(relationsCounter);
        }
    }
    else if ( newRelationName.isEmpty() && ok ){
        QMessageBox::critical(this, tr("Error"),
                              tr("You did not type a name for this new relation"),
                              QMessageBox::Ok, 0);
        addRelation();
    }
    else {
        statusMessage( QString(tr("New relation cancelled.")) );
        return;
    }
    statusMessage( QString(tr("New relation named %1, added."))
                   .arg( newRelationName ) );
}



/**
    Calls Graph::createVertex method to add a new RANDOM node into the activeGraph.
    Called when "Create Node" button is clicked on the Main Window.
*/
void MainWindow::addNode() {
    qDebug() << "MW::addNode() ";
    // minus a  screen edge offset...
    activeGraph.createVertex (
                -1, graphicsWidget->width()-10,  graphicsWidget->height()-10);
    statusMessage( tr("New node (numbered %1) added.")
                   .arg(activeGraph.lastVertexNumber())  );
}


/**
    Calls Graph::createVertex method to add a new node into the activeGraph.
    Called on double clicking
*/
void MainWindow::addNodeWithMouse(int num, QPointF p) {
    qDebug("MW: addNodeWithMouse(). Calling activeGraph::createVertex() for a vertex named %i", num);
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
    QString fn = QFileDialog::getSaveFileName(
                this,tr("Save"),
                getLastPath(), tr("Image Files (*.png)"));
    if (fn.isEmpty())  {
        statusMessage( tr("Saving aborted") );
        return false;
    }
    setLastPath(fn); // store this path
    tempFileNameNoPath=fn.split ("/");
    qDebug("slotExportPNG: grabbing canvas");
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
        QMessageBox::information(this,
                                 "Export to PNG...",
                                 tr("Image Saved as: ")+tempFileNameNoPath.last(), "OK",0);
    }
    else {
        picture.toImage().save(fn+".png", "PNG");
        QMessageBox::information(this,
                                 "Export to PNG...",
                                 tr("Image Saved as: ")+tempFileNameNoPath.last()+".png" , "OK",0);
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
    QString fn = QFileDialog::getSaveFileName(
                this,tr("Save Image as"), getLastPath(),tr("Image Files (*.bmp)"));
    if (fn.isEmpty())  {
        statusMessage( tr("Saving aborted") );
        return false;
    }
    setLastPath(fn); // store this path
    tempFileNameNoPath=fn.split ("/");

    QPixmap picture;
    qDebug("slotExportBMP: grabbing canvas");
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
        QMessageBox::information(this, tr("Export to BMP..."),
                                 tr("Image Saved as: ")+tempFileNameNoPath.last(), "OK",0);
    }
    else {
        picture.toImage().save(fn+"."+format, format.toLatin1());
        QMessageBox::information(this, tr("Export to BMP..."),
                                 tr("Image Saved as: ")+tempFileNameNoPath.last()+"."+format , "OK",0);
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

    QString m_fileName = QFileDialog::getSaveFileName(
                this, tr("Export to PDF"), getLastPath(),
                tr("Portable Document Format files (*.pdf)"));
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
    setLastPath(m_fileName);
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
    QString fn =  QFileDialog::getSaveFileName(
                this,
                tr("Export Network to File Named..."),
                getLastPath(), tr("Pajek (*.paj *.net *.pajek);;All (*)") );
    if (!fn.isEmpty())  {
        if  ( QFileInfo(fn).suffix().isEmpty() ){
            QMessageBox::information(this, "Missing Extension ",tr("File extension was missing! \nI am appending a standard .paj to the given filename."), "OK",0);
            fn.append(".paj");
        }
        fileName=fn;
        setLastPath(fileName);
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
    QString fn =  QFileDialog::getSaveFileName(
                this,
                tr("Export Network to File Named..."),
                getLastPath(), tr("Adjacency (*.adj *.sm *.txt *.csv *.net);;All (*)") );
    if (!fn.isEmpty())  {
        if  ( QFileInfo(fn).suffix().isEmpty() ){
            QMessageBox::information(this, "Missing Extension ",tr("File extension was missing! \nI am appending a standard .adj to the given filename."), "OK",0);
            fn.append(".adj");
        }
        fileName=fn;
        setLastPath(fileName);
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
        QString fn = QFileDialog::getSaveFileName(
                    this, "Export UCINET", getLastPath(), 0);
        if (!fn.isEmpty())  {
            fileName=fn;
            setLastPath(fileName);
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
        QString fn = QFileDialog::getSaveFileName(
                    this, "Export GW", getLastPath(), 0);
        if (!fn.isEmpty())  {
            fileName=fn;
            setLastPath(fileName);
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
        QString fn = QFileDialog::getSaveFileName(
                    this, "Export List", getLastPath(), 0);
        if (!fn.isEmpty())  {
            fileName=fn;
            setLastPath(fileName);
        }
        else  {
            statusMessage( tr("Saving aborted"));
            return false;
        }
    }

    return true;
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
    QString fn = dataDir + "socnetv-report-adjacency-matrix.dat";

    activeGraph.writeAdjacencyMatrix(fn, networkName.toLocal8Bit()) ;

    //Open a text editor window for the new file created by graph class
    TextEditor *ed = new TextEditor(fn);
    tempFileNameNoPath=fn.split( "/");
    ed->setWindowTitle(tempFileNameNoPath.last());
    ed->show();
    statusMessage(tr("Adjacency Matrix saved at ") + tempFileNameNoPath.last());
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

    //initNet();

    qDebug()<< "slotRecreateDataSet() datadir+fileName: " << dataDir+m_fileName;
    activeGraph.writeDataSetToFile(dataDir, m_fileName);

    if (m_fileName.endsWith(".graphml")) {
        m_fileFormat=1;
    }
    else if (m_fileName.endsWith(".pajek") || m_fileName.endsWith(".paj") ||
             m_fileName.endsWith(".net")) {
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
    checkSelectFileType = false;
    if ( loadNetworkFile(dataDir+m_fileName, "UTF-8", m_fileFormat) ) {
        qDebug() << "slotRecreateDataSet() loaded file " << m_fileName;
        fileName=m_fileName;
        previous_fileName=fileName;
        setWindowTitle("SocNetV "+ VERSION +" - "+fileName);
        QString message=tr("Dataset loaded. Dataset file saved as ") + fileName;
        statusMessage( message );
    }
    else {
        statusMessage( "Could not read new network data file. Aborting.");
    }
}

/**
    Calls activeGraph.createRandomNetErdos () to create a symmetric network
    Edge existance is controlled by a user specified possibility.
*/
void MainWindow::slotCreateRandomErdosRenyi(){

    statusMessage( "Creating a random symmetric network... ");

    m_randErdosRenyiDialog = new RandErdosRenyiDialog(this);

    connect( m_randErdosRenyiDialog, &RandErdosRenyiDialog::userChoices,
             this, &MainWindow::createRandomNetErdos );


    m_randErdosRenyiDialog->exec();


}






void MainWindow::createRandomNetErdos( const int newNodes,
                                       const QString model,
                                       const int edges,
                                       const float eprob,
                                       const QString mode,
                                       const bool diag)
{
    qDebug() << "MW::createRandomNetErdos()";

    statusMessage( "Erasing any existing network. ");
    initNet();

    statusMessage( tr("Creating random network. Please wait... ")  );


    if (showProgressBarAct->isChecked() && newNodes > 500 && eprob > 0.06){
        progressDialog= new QProgressDialog(
                    "Creating random network. \n "
                    " Please wait (or disable me from Options > View > ProgressBar, next time ;)).",
                    "Cancel", 0, newNodes+newNodes, this);
        progressDialog -> setWindowModality(Qt::WindowModal);
        connect( &activeGraph, SIGNAL( updateProgressDialog(int) ), progressDialog, SLOT(setValue(int) ) ) ;
        progressDialog->setMinimumDuration(0);
    }

    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

    activeGraph.createRandomNetErdos ( newNodes,
                                       model,
                                       edges,
                                       eprob,
                                       mode,
                                       diag);

    QApplication::restoreOverrideCursor();

    if (showProgressBarAct->isChecked() && newNodes > 500 && eprob > 0.06)
        progressDialog->deleteLater();

    fileLoaded=false;

    graphChanged();

    setWindowTitle("Untitled");
    double threshold = log(newNodes)/newNodes;

    //float avGraphDistance=activeGraph.averageGraphDistance();

    float clucof=activeGraph.clusteringCoefficient();

    if ( (eprob ) > threshold )
        QMessageBox::information(
                    this,
                    "New Random Network",
                    tr("Random network created. \n")+
                    tr("\nNodes: ")+ QString::number(activeNodes())+
                    tr("\nEdges: ")+  QString::number( (mode == "graph") ? activeEdges()/2.0 : activeEdges() ) +
                    //tr("\nAverage path length: ") + QString::number(avGraphDistance)+
                    tr("\nClustering coefficient: ")+QString::number(clucof)+
                    tr("\n\nOn the average, edges should be ") +
                    QString::number( eprob * newNodes*(newNodes-1)) +
                    tr("\nThis graph is almost surely connected because: \nprobability > ln(n)/n, that is: \n")
                    + QString::number(eprob)+
                    tr(" bigger than ")+ QString::number(threshold) , "OK",0);

    else
        QMessageBox::information(
                    this,
                    "New Random Network",
                    tr("Random network created. \n")+
                    tr("\nNodes: ")+ QString::number(activeNodes())+
                    tr("\nEdges: ")+  QString::number(  (mode == "graph") ? activeEdges()/2.0 : activeEdges()  )+
                    //tr("\nAverage path length: ") + QString::number(avGraphDistance)+
                    tr("\nClustering coefficient: ")+QString::number(clucof)+
                    tr("\n\nOn the average, edges should be ")
                    + QString::number(eprob * newNodes*(newNodes-1)) +
                    tr("\nThis graph is almost surely not connected because: \nprobability < ln(n)/n, that is: \n") +
                    QString::number(eprob)+ " smaller than "+ QString::number(threshold) , "OK",0);

    statusMessage( "Random network created. ");

}






/**
    Creates a pseudo-random network where every node has the same degree
*/
void MainWindow::slotCreateRegularRandomNetwork(){
    bool ok;
    statusMessage( "Creating a pseudo-random network where each node has the same degree... ");
    int newNodes= QInputDialog::getInt(
                       this,
                       tr("Create k-regular network"),
                       tr("This will create a network with nodes of the same degree d.")
                          + tr("\nPlease enter the number of nodes:"),
                       100, 1, maxNodes, 1, &ok
                ) ;
    if (!ok) {
        statusMessage( "You did not enter an integer. Aborting.");
        return;
    }
    int degree = QInputDialog::getInt(
                this,
                tr("Create k-regular network..."),
                tr("Now, select an even number d. \nThis will be the degree (number of edges) of each node:"),
                2, 2, newNodes-1, 2, &ok
                );

    if ( (degree% 2)==1 ) {
        QMessageBox::critical(
                    this,
                    "Error",
                    tr(" Sorry. I cannot create such a network. Degree must be even number"),
                    "OK",0 );
        return;
    }
    statusMessage( "Erasing any existing network. ");
    initNet();
    statusMessage( "Creating a pseudo-random network where each node has the same degree... ");

    if (showProgressBarAct->isChecked() && newNodes > 300){
        progressDialog= new QProgressDialog (
                    "Creating random network. Please wait (or disable me from Options > View > ProgressBar, next time ;)).", "Cancel", 0, (int) (newNodes+newNodes), this);
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
    statusMessage( "Uniform random network created: "
                   +QString::number(activeNodes())+" Nodes, "
                   +QString::number( activeEdges())+" Edges");

}




void MainWindow::slotCreateRandomGaussian(){
    graphChanged();

}




void MainWindow::slotCreateRandomScaleFree() {
    qDebug() << "MW;:slotCreateRandomScaleFree()";
    m_randScaleFreeDialog = new RandScaleFreeDialog(this);

    connect( m_randScaleFreeDialog, &RandScaleFreeDialog::userChoices,
             this, &MainWindow::createScaleFreeNetwork);

    m_randScaleFreeDialog->exec();

}


void MainWindow::createScaleFreeNetwork ( const int &nodes,
                                          const int &power,
                                          const int &initialNodes,
                                          const int &edgesPerStep,
                                          const float &zeroAppeal,
                                          const QString &mode)
{
    qDebug() << "MW;:createScaleFreeNetwork()";
    statusMessage( tr("Erasing any existing network. "));
    initNet();
    statusMessage( tr("Creating small world network. Please wait..."));

    double x0=scene->width()/2.0;
    double y0=scene->height()/2.0;
    double radius=(graphicsWidget->height()/2.0)-50;          //pixels

    if (showProgressBarAct->isChecked() && nodes > 300){
        progressDialog= new QProgressDialog(
                    tr("Creating random network. Please wait \n (or disable me from Options > View > ProgressBar, next time )."),
                    "Cancel", 0, (int) (2* nodes), this);
        progressDialog -> setWindowModality(Qt::WindowModal);
        connect( &activeGraph, SIGNAL( updateProgressDialog(int) ), progressDialog, SLOT(setValue(int) ) ) ;
        progressDialog->setMinimumDuration(0);
    }
    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

    activeGraph.createRandomNetScaleFree( nodes,
                                          power,
                                          initialNodes,
                                          edgesPerStep,
                                          zeroAppeal,
                                          mode,
                                          x0,
                                          y0,
                                          radius);

    QApplication::restoreOverrideCursor();

    if (showProgressBarAct->isChecked() && nodes > 300 )
        progressDialog->deleteLater();

    fileLoaded=false;

    graphChanged();
    setWindowTitle("Untitled");
    statusMessage( tr("Scale-free random network created: ")
                   + QString::number(activeNodes())
                   + " nodes, "+QString::number( activeEdges())+" edges");
    //float avGraphDistance=activeGraph.averageGraphDistance();
    float clucof=activeGraph.clusteringCoefficient();
    QMessageBox::information(this, "New scale-free network",
                             tr("Scale-free random network created.\n")+
                             tr("\nNodes: ")+ QString::number(activeNodes())+
                             tr("\nEdges: ")
                             +  QString::number( (mode == "graph" ) ? activeEdges()/2.0 : activeEdges())
                             //+  tr("\nAverage path length: ") + QString::number(avGraphDistance)
                             + tr("\nClustering coefficient: ")+QString::number(clucof)
                             , "OK",0);

}


void MainWindow::slotCreateRandomSmallWorld()
{
    qDebug() << "MW;:slotCreateRandomSmallWorld()";
    m_randSmallWorldDialog = new RandSmallWorldDialog(this);

    connect( m_randSmallWorldDialog, &RandSmallWorldDialog::userChoices,
             this, &MainWindow::createSmallWorldNetwork);


    m_randSmallWorldDialog->exec();

}

void MainWindow::createSmallWorldNetwork (const int &nodes,
                                            const int &degree,
                                            const float &beta,
                                            const QString &mode,
                                            const bool &diag)
{
    qDebug() << "MW;:createSmallWorldNetwork()";
    statusMessage( tr("Erasing any existing network. "));
    initNet();
    statusMessage( tr("Creating small world network. Please wait..."));

    double x0=scene->width()/2.0;
    double y0=scene->height()/2.0;
    double radius=(graphicsWidget->height()/2.0)-50;          //pixels

    if (showProgressBarAct->isChecked() && nodes > 300){
        progressDialog= new QProgressDialog(
                    tr("Creating random network. Please wait \n (or disable me from Options > View > ProgressBar, next time )."),
                    "Cancel", 0, (int) (2* nodes), this);
        progressDialog -> setWindowModality(Qt::WindowModal);
        connect( &activeGraph, SIGNAL( updateProgressDialog(int) ), progressDialog, SLOT(setValue(int) ) ) ;
        progressDialog->setMinimumDuration(0);
    }
    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
    activeGraph.createRandomNetSmallWorld(nodes, degree, beta, x0, y0, radius);
    activeGraph.symmetrize();
    QApplication::restoreOverrideCursor();

    if (showProgressBarAct->isChecked() && nodes > 300 )
        progressDialog->deleteLater();

    fileLoaded=false;

    graphChanged();
    setWindowTitle("Untitled");
    statusMessage( tr("Small world random network created: ")+QString::number(activeNodes())+" nodes, "+QString::number( activeEdges())+" edges");
    //float avGraphDistance=activeGraph.averageGraphDistance();
    float clucof=activeGraph.clusteringCoefficient();
    QMessageBox::information(this, "New Small World",
                             tr("Small world network created.\n")+
                             tr("\nNodes: ")+ QString::number(activeNodes())+
                             tr("\nEdges: ")
                              +  QString::number( (mode == "graph" ) ? activeEdges()/2.0 : activeEdges())
                             //+  tr("\nAverage path length: ") + QString::number(avGraphDistance)
                             + tr("\nClustering coefficient: ")+QString::number(clucof)
                             , "OK",0);

}





/**
    Creates a lattice network, i.e. a connected network where every node
    has the same degree and is Edgeed with its neighborhood.
*/
void MainWindow::slotCreateRandomRingLattice(){
    bool ok;
    statusMessage( "You have selected to create a ring lattice network. ");
    int newNodes=( QInputDialog::getInt(
                       this,
                       tr("Create ring lattice"),
                       tr("This will create a ring lattice network, where each node has degree d:\n d/2 edges to the right and d/2 to the left.\n Please enter the number of nodes you want:"),
                       100, 4, maxNodes, 1, &ok ) ) ;
    if (!ok) {
        statusMessage( "You did not enter an integer. Aborting.");
        return;
    }
    int degree = QInputDialog::getInt(
                this,
                tr("Create ring lattice..."),
                tr("Now, enter an even number d. \nThis is the total number of edges each new node will have:"),
                2, 2, newNodes-1, 2, &ok);
    if ( (degree% 2)==1 ) {
        QMessageBox::critical(this, "Error",tr(" Sorry. I cannot create such a network. Degree must be even number"), "OK",0);
        return;
    }

    statusMessage( "Erasing any existing network. ");
    initNet();
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

    statusMessage( "Ring lattice random network created: "+QString::number(activeNodes())+" nodes, "+QString::number( activeEdges())+" edges");

    setWindowTitle("Untitled");
    //float avGraphDistance=activeGraph.averageGraphDistance();
    //float clucof=activeGraph.clusteringCoefficient();
    QMessageBox::information(this, "Ring Lattice",
                             tr("Ring lattice network created.\n")+
                             tr("\nNodes: ")+ QString::number(activeNodes())+
                             tr("\nEdges: ")+  QString::number( activeEdges()/2.0)
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
void MainWindow::slotWebCrawl ( QString  seed, int maxNodes, int maxRecursion,
                                bool extLinks, bool intLinks) {
    this->slotFileClose();
    activeGraph.webCrawl( seed, maxNodes, maxRecursion,  extLinks, intLinks) ;

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

    if ( markedNodesExist ) {				// if a node has been already marked
        graphicsWidget->setMarkedNode(""); 	// call setMarkedNode to just unmark it.
        markedNodesExist=false;
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
            markedNodesExist=true;
            statusMessage( tr("Node found and marked. Press Ctrl+F again to unmark...") );
        }
        else {
            QMessageBox::information(this, tr("Find Node"),
                                     tr("Sorry. There is no such node in this network. \n Try again."), "OK",0);
        }
    }
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
    edgesLCD->display(activeEdges());
    densityLCD->display( activeGraph.density() );
}



void MainWindow::slotSelectAll(){
    qDebug() << "MainWindow::slotSelectAll()";
    graphicsWidget->selectAll();
    statusMessage( QString(tr("Selected nodes: %1") )
                   .arg( selectedNodes().count() ) );

}


void MainWindow::slotSelectNone(){
    qDebug() << "MainWindow::slotSelectNone()";
    graphicsWidget->selectNone();
    statusMessage( QString(tr("Selection cleared") ) );
}


/**
     Popups a context menu with some options when the user right-clicks on a node
*/
void MainWindow::openNodeContextMenu() {
    clickedJimNumber=clickedJim->nodeNumber();
    qDebug("MW: openNodeContextMenu() for node %i at %i, %i",
           clickedJimNumber, QCursor::pos().x(), QCursor::pos().y());

    QMenu *nodeContextMenu = new QMenu(QString::number(clickedJimNumber), this);
    Q_CHECK_PTR( nodeContextMenu );  //displays "out of memory" if needed
    if ( selectedNodes().count() == 1) {
        nodeContextMenu -> addAction( tr("## NODE ") + QString::number(clickedJimNumber) + " ##  ");
    }
    else {
        nodeContextMenu -> addAction( tr("## NODE ") + QString::number(clickedJimNumber) + " ##  " + tr(" (selected nodes: ") + QString::number (selectedNodes().count() ) + ")");
    }

    nodeContextMenu -> addSeparator();
    nodeContextMenu -> addAction(addEdgeAct);
    nodeContextMenu -> addAction(removeNodeAct );
    nodeContextMenu -> addAction(propertiesNodeAct );
    //QCursor::pos() is good only for menus not related with node coordinates
    nodeContextMenu -> exec(QCursor::pos() );
    delete  nodeContextMenu;
    clickedJimNumber=-1;    //undo node selection
}



/**
     Popups a context menu with some options when the user right-clicks on an Edge
*/
void MainWindow::openEdgeContextMenu() {
    int source=clickedEdge->sourceNodeNumber();
    int target=clickedEdge->targetNodeNumber();
    qDebug("MW: openEdgeContextMenu() for edge %i-%i at %i, %i",source, target, QCursor::pos().x(), QCursor::pos().y());
    QString edgeName=QString::number(source)+QString("->")+QString::number(target);
    //make the menu
    QMenu *edgeContextMenu = new QMenu(edgeName, this);
    edgeContextMenu -> addAction( "## EDGE " + edgeName + " ##  ");
    edgeContextMenu -> addSeparator();
    edgeContextMenu -> addAction( removeEdgeAct );
    edgeContextMenu -> addAction( changeEdgeWeightAct );
    edgeContextMenu -> addAction( changeEdgeColorAct );
    edgeContextMenu -> exec(QCursor::pos() );
    delete  edgeContextMenu;
}

/**
     Popups a context menu with some options when the user right-clicks on the scene
*/
void MainWindow::openContextMenu( const QPointF &mPos) {
    cursorPosGW=mPos;
    QMenu *contextMenu = new QMenu(" Menu",this);
    Q_CHECK_PTR( contextMenu );  //displays "out of memory" if needed


    contextMenu -> addAction( "## Selected nodes: "
                              + QString::number(  selectedNodes().count() ) + " ##  ");

    contextMenu -> addSeparator();

    contextMenu -> addAction( addNodeAct );

    if (selectedNodes().count()) {
        contextMenu -> addAction(propertiesNodeAct );
    }

    contextMenu -> addAction( addEdgeAct );

    QMenu *options=new QMenu("Options", this);
    contextMenu -> addMenu(options );

    options -> addAction (changeBackColorAct  );
    options -> addAction (backgroundImageAct  );
    options -> addAction (changeAllNodesSizeAct );
    options -> addAction (changeAllNodesShapeAct  );
    options -> addAction (changeAllNodesColorAct );
    options -> addAction (changeAllEdgesColorAct  );
    options -> addAction (displayNodeNumbersAct);
    options -> addAction (displayNodeLabelsAct);
    //QCursor::pos() is good only for menus not related with node coordinates
    contextMenu -> exec(QCursor::pos() );
    delete  contextMenu;
    cursorPosGW=QPoint(-1,-1);
}




QList<QGraphicsItem *> MainWindow::selectedNodes() {
    return graphicsWidget->selectedItems();

}







/**
*	When the user clicks on a node, displays some information about it on the status bar.
*/
void MainWindow::nodeInfoStatusBar ( Node *jim) {
    qDebug ("MW: NodeInfoStatusBar()");
    edgeClicked=false;
    nodeClicked=true;
    clickedJim=jim;
    clickedJimNumber=clickedJim->nodeNumber();
    int inDegree=activeGraph.inDegree(clickedJimNumber);
    int outDegree=activeGraph.outDegree(clickedJimNumber);
    selectedNodeLCD->display (clickedJimNumber);
    inDegreeLCD->display (inDegree);
    outDegreeLCD->display (outDegree);
    if (activeGraph.vertices() < 500)
        clucofLCD->display(activeGraph.localClusteringCoefficient(clickedJimNumber));

    statusMessage(  QString(tr("(%1, %2);  Node %3, label %4 - "
                               "In-Degree: %5, Out-Degree: %6")).arg( ceil( clickedJim->x() ) )
                    .arg( ceil( clickedJim->y() )).arg( clickedJimNumber ).arg( clickedJim->labelText() )
                    .arg(inDegree).arg(outDegree) );
}



/**
*	When the user clicks on an Edge, displays some information about it on the status bar.
*/
void MainWindow::edgeInfoStatusBar (Edge* edge) {
    clickedEdge=edge;
    edgeClicked=true;
    nodeClicked=false;

    if (edge->isReciprocal()) {
        float outbound = activeGraph.hasArc
                (edge->sourceNodeNumber(), edge->targetNodeNumber());
        float inbound = activeGraph.hasArc
                (edge->targetNodeNumber(), edge->sourceNodeNumber());
        if (outbound==inbound)
            statusMessage(  QString
                        (tr("Symmetric edge %1 <--> %2 of weight %3 has been selected. "
                                   "Click again to unselect it."))
                    .arg( edge->sourceNodeNumber() ).arg(edge->targetNodeNumber())
                    .arg(edge->weight()) ) ;
        else
            statusMessage(  QString
                        (tr("Arc %1 --> %2 of weight %3 "
                            " and Arc %4 --> %5 of weight %6"
                            " have been selected. "
                                   "Click again to unselect them."))
                            .arg(edge->sourceNodeNumber() )
                            .arg(edge->targetNodeNumber())
                            .arg(outbound)
                            .arg( edge->targetNodeNumber() )
                            .arg(edge->sourceNodeNumber())
                            .arg(inbound) ) ;

    }
    else {
        statusMessage(  QString(tr("Arc %1 --> %2 of weight %3 has been selected. "
                                   "Click again to unselect it."))
                    .arg( edge->sourceNodeNumber() ).arg(edge->targetNodeNumber())
                    .arg(edge->weight()) ) ;
    }
}







/**
* 	Deletes a node and the attached objects (edges, etc).
*	It deletes clickedJim (signal from GraphicsView or set by another function) 
*	or else asks for a nodeNumber to remove. The nodeNumber is doomedJim.
    Called from nodeContextMenu
*/
void MainWindow::slotRemoveNode() {
    qDebug() << "MW: slotRemoveNode()";
    if (!activeGraph.vertices())  {
        QMessageBox::critical(
                    this,
                    "Error",
                    tr("Nothing to do! \n"
                       "Load a network file or add some nodes first."), "OK",0);
        statusMessage( tr("Nothing to remove.")  );
        return;
    }
    if (activeGraph.relations() > 1){
        QMessageBox::critical(
                    this, "Error",
                    tr("Cannot remove node! \n"
                       "This a network with more than 1 relations. If you remove "
                       "a node from the active relation, and then ask me to go "
                       "to the previous or the next relation, then I would crash "
                       "because I would try to display edges from a delete node."
                       "You can only add nodes in multirelational networks."),
                    "OK",0);
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
    else if (nodeClicked && clickedJimNumber >= 0 && clickedJimNumber<= max ) {
        doomedJim=clickedJimNumber ;
    }
    else if (!nodeClicked ) {
        doomedJim =  QInputDialog::getInt(this,"Remove node",tr("Choose a node to remove between ("
                                                                + QString::number(min).toLatin1()+"..."+QString::number(max).toLatin1()+"):"),min, 1, max, 1, &ok);
        if (!ok) {
            statusMessage( "Remove node operation cancelled." );
            return;
        }
    }
    qDebug ("MW: removing vertex with number %i from Graph", doomedJim);
    activeGraph.removeVertex(doomedJim);
    clickedJimNumber=-1;
    nodeClicked=false;
    graphChanged();
    qDebug("MW: removeNode() completed. Node %i removed completely.",doomedJim);
    statusMessage( tr("Node removed completely. Ready. ") );
}


void MainWindow::slotChangeNodeProperties() {

    qDebug() << "MW::slotChangeNodeProperties()";
//    if (!fileLoaded && !networkModified )  {
    if (!activeGraph.vertices())  {
        QMessageBox::critical(
                    this,
                    "Error",
                    tr("Nothing to do! \n"
                       "Load a network file or add some nodes first."), "OK",0);
        statusMessage( tr("Nothing to remove.")  );
        return;
    }
    int min=-1, max=-1, size = initNodeSize;
    QColor color = QColor(initNodeColor);
    QString shape= initNodeShape, label="";
    bool ok=false;


    if ( selectedNodes().count() == 0) {
        min = activeGraph.firstVertexNumber();
        max = activeGraph.lastVertexNumber();
        qDebug("MW: min is %i and max is %i", min, max);
        if (min==-1 || max==-1 ) {
            qDebug("ERROR in finding min max nodeNumbers. Abort");
            return;
        }

        clickedJimNumber =  QInputDialog::getInt(
                    this,
                    "Node Properties",
                    tr("Choose a node between ("
                    + QString::number(min).toLatin1()
                    +"..."
                    + QString::number(max).toLatin1()+"):"),min, 1, max, 1, &ok);
        if (!ok) {
            statusMessage( "Node properties cancelled." );
            return;
        }
    }
    else   {
        foreach (QGraphicsItem *item, selectedNodes() ) {
           if ( (clickedJim = qgraphicsitem_cast<Node *>(item) )) {
               if ( selectedNodes().count() > 1 ) {
                   clickedJimNumber = clickedJim->nodeNumber();
                   color = activeGraph.vertexColor( clickedJimNumber );
                   shape = activeGraph.vertexShape( clickedJimNumber);
                   size = activeGraph.vertexSize ( clickedJimNumber);
               }
               else {
                    clickedJimNumber = clickedJim->nodeNumber();
                    label = activeGraph.vertexLabel( clickedJimNumber );
                    color = activeGraph.vertexColor( clickedJimNumber );
                    shape = activeGraph.vertexShape( clickedJimNumber);
                    size = activeGraph.vertexSize ( clickedJimNumber);
               }
           }
        }
    }
    qDebug ()<< "MW: changing properties for "<< clickedJimNumber ;

    m_nodeEditDialog = new NodeEditDialog(this, label, size, color, shape) ;

    connect( m_nodeEditDialog, &NodeEditDialog::userChoices,
             this, &MainWindow::slotNodeProperties );

    m_nodeEditDialog->exec();

    statusMessage( tr("Node properties dialog opened. Ready. ") );
}



void MainWindow::slotNodeProperties( const QString label, const int size,
                                     const QString value, const QColor color,
                                     const QString shape) {
    qDebug()<< "MW::slotNodeProperties() "
            << " label " << label
            << " size " << size
            << "value " << value
            << " color " << color
            << " shape " << shape
               << " clickedJimNumber " <<clickedJimNumber
                  << " selectedNodes " << selectedNodes().count();

    foreach (QGraphicsItem *item, selectedNodes() ) {
        if ( (clickedJim = qgraphicsitem_cast<Node *>(item) )) {

            clickedJimNumber = clickedJim->nodeNumber();
            if ( selectedNodes().count() > 1 )
            {
                activeGraph.setVertexLabel(
                            clickedJimNumber,
                            label + QString::number(clickedJimNumber)
                            );
            }
            else
                activeGraph.setVertexLabel(
                            clickedJimNumber,
                            label
                            );

            if (!showLabels())
                displayNodeLabelsAct->setChecked(true);

            qDebug () <<  clickedJimNumber;
            qDebug()<<"MW: updating color ";
            activeGraph.setVertexColor( clickedJimNumber, color.name());
            qDebug()<<"MW: updating size ";
            activeGraph.setVertexSize(clickedJimNumber,size);
            qDebug()<<"MW: updating shape ";
            activeGraph.setVertexShape( clickedJimNumber, shape);
            clickedJim->setShape(shape);
        }
    }
    clickedJim=0;
    clickedJimNumber=-1;

    graphChanged();
    statusMessage( tr("Ready. "));

}

/**
*	Adds a new edge between two nodes specified by the user.
    Called when user clicks on the MW button "Add edge".
*/
void MainWindow::slotAddEdge(){
    qDebug ("MW: slotAddEdge()");
    if (!fileLoaded && !networkModified )  {
        QMessageBox::critical(this, "Error",tr("No nodes!! \nCreate some nodes first."), "OK",0);
        statusMessage( tr("There are no nodes yet...")  );
        return;
    }

    int sourceNode=-1, targetNode=-1, sourceIndex=-1, targetIndex=-1;
    float weight=1; 	//weight of this new edge should be one...
    bool ok=false;
    int min=activeGraph.firstVertexNumber();
    int max=activeGraph.lastVertexNumber();

    if (min==max) return;		//if there is only one node -> no edge

    if (!nodeClicked) {
        sourceNode=QInputDialog::getInt(this, "Create new edge, Step 1",tr("This will draw a new edge between two nodes. \nEnter source node ("+QString::number(min).toLatin1()+"..."+QString::number(max).toLatin1()+"):"), min, 1, max , 1, &ok ) ;
        if (!ok) {
            statusMessage( "Add edge operation cancelled." );
            return;
        }
    }
    else sourceNode=clickedJimNumber;
    qDebug () << "sourceNode=clickedJimNumber " << clickedJimNumber;
    if ( (sourceIndex =activeGraph.hasVertex(sourceNode)) ==-1 ) {
        statusMessage( tr("Aborting. ")  );
        QMessageBox::critical(this,"Error","No such node.", "OK",0);
        qDebug ("MW: slotAddEdge: Cant find sourceNode %i.", sourceNode);
        return;
    }

    targetNode=QInputDialog::getInt
            (this, "Create new edge, Step 2",
             tr("Source node accepted. \nNow enter target node ("+
                QString::number(min).toLatin1()+"..."+QString::number(max)
                .toLatin1()+"):"),min, min, max , 1, &ok)     ;
    if (!ok) {
        statusMessage( "Add edge target operation cancelled." );
        return;
    }
    if ( (targetIndex=activeGraph.hasVertex(targetNode)) ==-1 ) {
        statusMessage( tr("Aborting. ")  );
        QMessageBox::critical(this,"Error","No such node.", "OK",0);
        qDebug ("MW: slotAddEdge: Cant find targetNode %i",targetNode);
        return;
    }

    weight=QInputDialog::getDouble(
                this, "Create new edge, Step 3",
                tr("Source and target nodes accepted. \n "
                   "Please, enter the weight of new edge: "),1.0, -100.0, 100.0, 1, &ok);
    if (!ok) {
        statusMessage( "Add edge operation cancelled." );
        return;
    }
    //Check if this edge already exists...
    if (activeGraph.hasArc(sourceNode, targetNode)!=0 ) {
        qDebug("edge exists. Aborting");
        statusMessage( tr("Aborting. ")  );
        QMessageBox::critical(this,"Error","edge already exists.", "OK",0);
        return;
    }

    addEdge(sourceNode, targetNode, weight);
    graphChanged();
    statusMessage( tr("Ready. ")  );
}



/** 	
    helper to slotAddEdge() above
    Also called from GW::userMiddleClicked() signal when user creates edges with middle-clicks
    Calls Graph::createEdge method to add the new edge to the active Graph
*/
void MainWindow::addEdge (int v1, int v2, float weight) {
    qDebug("MW: addEdge() - setting user preferences and calling Graph::createEdge(...)");
    bool drawArrows=displayEdgesArrowsAct->isChecked();
    int reciprocal=0;
    bool bezier = false;
    activeGraph.createEdge(v1, v2, weight, reciprocal, drawArrows, bezier);

    if ( activeEdges() == 1 && changeRelationCombo->count() == 0 ) {
        addRelation();
    }
}


/**
*	Erases the clicked edge. Otherwise asks the user to specify one edge.
*	First deletes arc reference from object nodeVector
*	then deletes arc item from scene
**/
void MainWindow::slotRemoveEdge(){
    if ( (!fileLoaded && !networkModified) || activeEdges() ==0 )  {
        QMessageBox::critical(this, "Error",tr("There are no edges! \nLoad a network file or create a new network first."), "OK",0);
        statusMessage( tr("No edges to remove - sorry.")  );
        return;
    }

    int min=0, max=0, sourceNode=-1, targetNode=-1;
    bool ok=false;
    min=activeGraph.firstVertexNumber();
    max=activeGraph.lastVertexNumber();

    if (!edgeClicked) {
        sourceNode=QInputDialog::getInt(
                    this,tr("Remove edge"),
                    tr("Source node:  (")+QString::number(min)+
                    "..."+QString::number(max)+"):", min, 1, max , 1, &ok )   ;
        if (!ok) {
            statusMessage( "Remove edge operation cancelled." );
            return;
        }

        targetNode=QInputDialog::getInt(this, tr("Remove edge"), tr("Target node:  (")+QString::number(min)+"..."+QString::number(max)+"):",min, 1, max , 1, &ok )   ;
        if (!ok) {
            statusMessage( "Remove edge operation cancelled." );
            return;
        }
        if ( activeGraph.hasArc(sourceNode, targetNode)!=0 ) {
            if (activeGraph.symmetricEdge(sourceNode, targetNode) )
                graphicsWidget->unmakeEdgeReciprocal(targetNode, sourceNode);
            graphicsWidget->eraseEdge(sourceNode, targetNode);
            activeGraph.removeEdge(sourceNode, targetNode);
        }
        else {
            QMessageBox::critical(this, "Remove edge",tr("There is no such edge."), "OK",0);
            statusMessage( tr("There are no nodes yet...")  );
            return;
        }

    }
    else {
        sourceNode = clickedEdge->sourceNodeNumber();
        targetNode = clickedEdge->targetNodeNumber();
        if (activeGraph.symmetricEdge(sourceNode, targetNode) ) {
            QString s=QString::number(sourceNode);
            QString t=QString::number(targetNode);
            switch (QMessageBox::information( this, tr("Remove edge"),
                                              tr("This edge is directed. \n") +
                                              tr("Select what Direction to delete or Both..."),
                                              s+" -> "+ t, t+" -> "+s, tr("Both"), 0, 1 ))

            {
            case 0:
                graphicsWidget->removeItem(clickedEdge);
                activeGraph.removeEdge(sourceNode, targetNode);
                //make new edge
                // 						graphicsWidget->unmakeEdgeReciprocal(clickedEdge->targetNodeNumber(), clickedEdge->sourceNodeNumber());
                //FIXME weight should be the same
                graphicsWidget->drawEdge(targetNode, sourceNode, 1, false, displayEdgesArrowsAct->isChecked(), initEdgeColor, false);

                break;
            case 1:
                clickedEdge->unmakeReciprocal();
                //graphicsWidget->removeItem(clickedEdge);
                activeGraph.removeEdge(targetNode, sourceNode);
                //						graphicsWidget->drawEdge(i, j, false, drawArrowsAct->isChecked(), initEdgeColor, false);
                break;
            case 2:
                graphicsWidget->removeItem(clickedEdge);
                activeGraph.removeEdge(sourceNode, targetNode);
                activeGraph.removeEdge(targetNode, sourceNode);
            }


        }
        else {
            graphicsWidget->removeItem(clickedEdge);
            activeGraph.removeEdge(sourceNode, targetNode);

        }


    }
    graphChanged();
    qDebug("MW: View items now: %i ", graphicsWidget->items().size());
    qDebug("MW: Scene items now: %i ", scene->items().size());
}








/**
*  Changes the color of all nodes
*/
void MainWindow::slotAllNodesColor(){
    QColor color = QColorDialog::getColor( Qt::red, this,
                                           "Change the color of all nodes" );
    if (color.isValid()) {
        initNodeColor=color.name();
        QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
        qDebug() << "MainWindow::slotAllNodesColor() : " << initNodeColor;
        activeGraph.setAllVerticesColor(initNodeColor);
        QApplication::restoreOverrideCursor();
        statusMessage( tr("Ready. ")  );
    }
    else {
        // user pressed Cancel
        statusMessage( tr("Nodes color change aborted. ") );
    }
}







//TODO slotChangeEdgeLabel
void MainWindow::slotChangeEdgeLabel(){
    graphChanged();
}



/**
*	Changes the colour of the clicked edge.
*	If no edge is clicked, then it asks the user to specify one.
*/
void MainWindow::slotChangeEdgeColor(){
    qDebug() << "MW::slotChangeEdgeColor()";
    if ( ( !fileLoaded && !networkModified) || activeEdges() ==0 )  {
        QMessageBox::critical(this, "Error",
                              tr("There are no edges! \nLoad a network file or create a new network first."), "OK",0);
        statusMessage( tr("No edges present...")  );
        return;
    }

    int sourceNode=-1, targetNode=-1;
    bool ok=false;
    QString newColor;
    int min=activeGraph.firstVertexNumber();
    int max=activeGraph.lastVertexNumber();

    if (!edgeClicked) {	//no edge clicked. Ask user to define an edge.
        sourceNode=QInputDialog::getInt(this,
                                        "Change edge color",
                                        tr("Select edge source node:  ("+
                                           QString::number(min).toLatin1()+
                                           "..."+QString::number(max).toLatin1()+
                                           "):"), min, 1, max , 1, &ok)   ;
        if (!ok) {
            statusMessage( "Change edge color operation cancelled." );
            return;
        }
        targetNode=QInputDialog::getInt(this,
                                        "Change edge color...",
                                        tr("Select edge target node:  ("+
                                         QString::number(min).toLatin1()+"..." +
                                         QString::number(max).toLatin1()+"):"),
                                        min, 1, max , 1, &ok  )   ;
        if (!ok) {
            statusMessage( "Change edge color operation cancelled." );
            return;
        }

        if ( ! activeGraph.hasArc (sourceNode, targetNode ) )  {
             statusMessage( tr("There is no such edge. ") );
             QMessageBox::critical(this, "Error",
                                   tr("No edge! \nNo such edge found in current network."), "OK",0);

             return;
        }


        QColor color = QColorDialog::getColor(
                    Qt::black, this, tr("Select new color....") );
        if ( color.isValid()) {
            QString newColor=color.name();
            qDebug() << "MW::slotChangeEdgeColor() to " << newColor;
            activeGraph.setEdgeColor( sourceNode, targetNode, newColor);
            statusMessage( tr("Ready. ")  );
        }
        else {
            statusMessage( tr("Change edge color aborted. ") );
        }

    }
    else {	//edge has been clicked. Just ask the color and call the appropriate methods.
        QColor color = QColorDialog::getColor(
                    Qt::black, this, tr("Select new color....") );
        if ( color.isValid()) {
            QString newColor=color.name();
            qDebug() << "MW::slotChangeEdgeColor() to " << newColor;
            activeGraph.setEdgeColor( clickedEdge->sourceNodeNumber(),
                                      clickedEdge->targetNodeNumber(), newColor);
            statusMessage( tr("Ready. ")  );
        }
        else {
            statusMessage( tr("Change edge color aborted. ") );
        }

    }
}




/**
*	Changes the weight of the clicked edge.
*	If no edge is clicked, asks the user to specify an Edge.
*/
void MainWindow::slotChangeEdgeWeight(){
    if ( ( !fileLoaded && !networkModified) || activeEdges() ==0 )  {
        QMessageBox::critical(this, "Error",tr("There are no edges! \nLoad a network file or create a new network first."), "OK",0);
        statusMessage( tr("No edges present...")  );
        return;
    }

    qDebug("MW::slotChangeEdgeWeight()");
    int  sourceNode=-1, targetNode=-1;
    float newWeight=1.0;
    int min=activeGraph.firstVertexNumber();
    int max=activeGraph.lastVertexNumber();

    bool ok=false;
    if (!edgeClicked) {
        sourceNode=QInputDialog::getInt(this, "Change edge weight",tr("Select edge source node:  ("+QString::number(min).toLatin1()+"..."+QString::number(max).toLatin1()+"):"), min, 1, max , 1, &ok)   ;
        if (!ok) {
            statusMessage( "Change edge weight operation cancelled." );
            return;
        }

        targetNode=QInputDialog::getInt(this, "Change edge weight...", tr("Select edge target node:  ("+QString::number(min).toLatin1()+"..."+QString::number(max).toLatin1()+"):"),min, 1, max , 1, &ok  )   ;
        if (!ok) {
            statusMessage( "Change edge weight operation cancelled." );
            return;
        }

        qDebug("source %i target %i",sourceNode, targetNode);

        QList<QGraphicsItem *> list=scene->items();
        for (QList<QGraphicsItem *>::iterator it=list.begin(); it!= list.end() ; it++)
            if ( (*it)->type()==TypeEdge) {
                Edge *edge=(Edge*) (*it);
                qDebug ("MW: searching edge...");
                if ( edge->sourceNodeNumber()==sourceNode && edge->targetNodeNumber()==targetNode ) {
                    qDebug("MW: edge found");
                    newWeight=(float) QInputDialog::getDouble(this,
                                                              "Change edge weight...",tr("New edge Weight: "), 1, -100, 100 ,1, &ok ) ;
                    if (ok) {
                        edge->setWeight(newWeight);
                        edge->update();
                        activeGraph.setArcWeight(sourceNode, targetNode, newWeight);
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
    else {  //edgeClicked
        qDebug() << "MW: slotChangeedgeWeight() - an Edge has already been clicked";
        sourceNode=clickedEdge->sourceNodeNumber();
        targetNode=clickedEdge->targetNodeNumber();
        qDebug() << "MW: slotChangeEdgeWeight() from "
                 << sourceNode << " to " << targetNode;
        if ( activeGraph.symmetricEdge(sourceNode, targetNode) ) {
            QString s=QString::number(sourceNode);
            QString t=QString::number(targetNode);
            switch (QMessageBox::information( this, tr("Change edge weight"),
                                              tr("This edge is reciprocal. \n") +
                                              tr("Select what Direction to change or Both..."),
                                              s+" -> "+ t, t+" -> "+s, tr("Both"), 0, 1 ))
            {
            case 0:
                qDebug("MW: slotChangeEdgeWeight()  real edge %i -> %i", sourceNode, targetNode);
                newWeight=QInputDialog::getDouble(this,
                                                  "Change edge weight...",tr("New edge weight: "), 1.0, -100.0, 100.00 ,1, &ok) ;
                if (ok) {
                    clickedEdge->setWeight(newWeight);
                    clickedEdge->update();
                    qDebug()<<"MW: newWeight will be "<< newWeight;
                    activeGraph.setArcWeight(sourceNode, targetNode, newWeight);
                    statusMessage(  QString(tr("Ready."))  );
                    return;
                }
                else {
                    statusMessage(  QString(tr("Change edge weight cancelled."))  );
                    return;
                }
                break;
            case 1:
                qDebug("MW: slotChangeEdgeWeight() virtual edge %i -> %i",targetNode , sourceNode);
                newWeight=(float) QInputDialog::getDouble(this,
                                                          "Change edge weight...",tr("New edge Weight: "), 1, -100, 100 ,1, &ok ) ;
                if (ok) {
                    qDebug()<<"MW: newWeight will be "<< newWeight;
                    activeGraph.setArcWeight( targetNode, sourceNode, newWeight);
                    statusMessage(  QString(tr("Ready."))  );
                    return;
                }
                else {
                    statusMessage(  QString(tr("Change edge weight cancelled."))  );
                    return;
                }
                break;
            case 2:
                qDebug("MW: slotChangeEdgeWeight()  both directions %i <-> %i",targetNode , sourceNode);
                newWeight=(float) QInputDialog::getDouble(this,
                                                          "Change edge weight...",tr("New edge Weight: "), 1, -100, 100 ,1, &ok ) ;

                if (ok) {
                    qDebug()<<"MW: Changing first direction. NewWeight will be "<< newWeight;
                    activeGraph.setArcWeight(sourceNode, targetNode, newWeight);
                    qDebug()<<"MW: Changing opposite direction. NewWeight will be "<< newWeight;
                    activeGraph.setArcWeight( targetNode, sourceNode, newWeight);
                    statusMessage(  QString(tr("Ready."))  );
                    return;
                }
                else {
                    statusMessage(  QString(tr("Change edge weight cancelled."))  );
                    return;
                }
                break;
            }
        }
        else {
            qDebug() << "MW: slotChangeEdgeWeight()  real edge " << sourceNode
                     << " -> " <<targetNode;
            newWeight=QInputDialog::getDouble(this,
                                              "Change edge weight...",tr("New edge weight: "), 1.0, -100, 100 ,1, &ok) ;
            if (ok) {
                qDebug() << "MW: slotChangeEdgeWeight()  setWeight to  "
                         << newWeight;
                clickedEdge->setWeight(newWeight);
                qDebug() << "MW: slotChangeEdgeWeight()  calling update  ";
                clickedEdge->update();
                qDebug()<<"MW: newWeight will be "<< newWeight;
                activeGraph.setArcWeight(sourceNode, targetNode, newWeight);
                statusMessage(  QString(tr("Ready."))  );
                return;
            }
            else {
                statusMessage(  QString(tr("Change edge weight cancelled."))  );
                return;
            }

        }
        edgeClicked=false;
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
 * @brief MainWindow::slotFilterIsolateNodes
 *Calls Graph::filterIsolateVertices to filter vertices with no edges
 */
void MainWindow::slotFilterIsolateNodes(bool checked){
    Q_UNUSED(checked);
    if (!fileLoaded && !networkModified  )  {
        QMessageBox::critical(this, "Error",tr("Nothing to filter! \nLoad a network file or create a new network. \nThen ask me to compute something!"), "OK",0);
        statusMessage(  QString(tr("Nothing to filter!"))  );
        return;
    }
    qDebug()<< "MW: slotFilterIsolateNodes";
    activeGraph.filterIsolateVertices( ! filterIsolateNodesAct->isChecked() );
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
*	Transforms all nodes to edges
    TODO slotTransformNodes2Edges
*/
void MainWindow::slotTransformNodes2Edges(){
    graphChanged();

}





/**
*	Converts all edges to double edges, so that the network becomes undirected (symmetric adjacency matrix).
*/
void MainWindow::slotSymmetrize(){
    if ( ( !fileLoaded && !networkModified) || activeEdges() ==0 )  {
        QMessageBox::critical(this, "Error",tr("There are no edges! \nLoad a network file or create a new network first."), "OK",0);
        statusMessage( tr("No edges present...")  );
        return;
    }
    qDebug("MW: slotSymmetrize() calling symmetrize");
    activeGraph.symmetrize();
    QMessageBox::information(this, "Symmetrize",tr("All edges are reciprocal. \nYour network is symmetric..."), "OK",0);
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
 * @brief MainWindow::slotLayoutRandom
 * to reposition all nodes on a circular layout randomly
 */
void MainWindow::slotLayoutRandom(){
    if (!fileLoaded && !networkModified  )  {
                QMessageBox::critical(
                    this, "Error",
                    tr("Sorry, I can't follow! "
                       "\nLoad a network file or create a new network first. \n"
                       "Then we can talk about layouts!"), "OK",0);
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
 * @brief MainWindow::slotLayoutCircularRandom
 */
void MainWindow::slotLayoutCircularRandom(){
    qDebug() << "MainWindow::slotLayoutCircularRandom()";
    if (!fileLoaded && !networkModified  )  {
        QMessageBox::critical(
                    this, "Error",
                    tr("Sorry, I can't follow! "
                       "\nLoad a network file or create a new network first. \n"
                       "Then we can talk about layouts!"), "OK",0);
        statusMessage(  QString(tr("Nothing to layout! Are you dreaming?"))  );
        return;
    }

    double x0=scene->width()/2.0;
    double y0=scene->height()/2.0;
    double maxRadius=(graphicsWidget->height()/2.0)-50;          //pixels
    statusMessage(  QString(tr("Calculating new nodes positions. Please wait...")) );
    graphicsWidget->clearGuides();
    createProgressBar();
    activeGraph.layoutCircularRandom(x0, y0, maxRadius);
    destroyProgressBar();
    statusMessage( tr("Nodes in random circles.") );
}





/**
        slotLayoutSpringEmbedder called from menu or toolbox checkbox
*/
void MainWindow::slotLayoutSpringEmbedder(bool state ){
    qDebug()<< "MW:slotLayoutSpringEmbedder";
    if (!fileLoaded && !networkModified  )  {
        QMessageBox::critical(this, "Error",tr("There are node nodes yet!\nLoad a network file or create a new network first. \nThen we can talk about layouts!"), "OK",0);
        statusMessage( tr("I am really sorry. You must really load a file first... ")  );
        layoutEadesBx->setCheckState(Qt::Unchecked);
        return;
    }

    //Stop any other layout running
    layoutFruchtermanBx->setCheckState(Qt::Unchecked);
    activeGraph.nodeMovement(!state, 2, graphicsWidget->width(), graphicsWidget->height());

    scene->setItemIndexMethod (QGraphicsScene::NoIndex); //best when moving items

    if (state){
        statusMessage( tr("Embedding a spring-gravitational model on the network.... ")  );
        layoutEadesBx->setCheckState(Qt::Checked);
        activeGraph.nodeMovement(state, 1, graphicsWidget->width(), graphicsWidget->height());
        statusMessage( tr("Click on the checkbox \"Spring-Embedder\" to stop movement!") );
    }
    else {
        layoutEadesBx->setCheckState(Qt::Unchecked);
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
    if (layoutFruchtermanBx->checkState() == Qt::Unchecked){
        statusMessage( tr("Embedding a repelling-attracting forces model on the network.... ")  );
        layoutFruchtermanBx->setCheckState(Qt::Checked);
        statusMessage( tr("Click on the checkbox \"Fruchterman-Reingold\" to stop movement!") );
    }
    else {
        layoutFruchtermanBx->setCheckState(Qt::Unchecked);
        statusMessage( tr("Movement stopped!") );
    }

}


/** 
    Called when user presses button.
    Calls Graph::startNodeMovement to embed a repelling-attracting forces layout...
*/
void MainWindow::layoutFruchterman (int state){
    qDebug("MW: layoutFruchterman ()");
    layoutEadesBx->setChecked(false);
    scene->setItemIndexMethod (QGraphicsScene::NoIndex); //best when moving items
    activeGraph.nodeMovement(state, 2, graphicsWidget->width(), graphicsWidget->height());
    scene->setItemIndexMethod (QGraphicsScene::BspTreeIndex); //best when not moving items
}



/**
 * @brief MainWindow::slotLayoutNodeSizesByOutDegree
 * Resizes all nodes according to their outDegree
 * Called when user selects the relevant menu entry or the option in the toolbox
 * @param checked
 */
void MainWindow::slotLayoutNodeSizesByOutDegree(bool checked){
    if (!fileLoaded && !networkModified  )  {
        QMessageBox::critical(
                    this, "Error",
                    tr("There are no nodes yet!\n"
                       "Load a network file or create a new network first. "
                       "Then we can talk about layouts!"), "OK",0);
        statusMessage( tr("I am really sorry. You must really load a file first... ")  );
        return;
    }

    qDebug("MW: slotLayoutNodeSizesByOutDegree()");

    if (checked != true) {
        qDebug("MW: slotLayoutNodeSizesByOutDegree() resetting size");
        nodeSizesByOutDegreeAct->setChecked(false);
        nodeSizesByOutDegreeBx->setChecked(false);

        QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

        activeGraph.layoutVerticesSizeByProminenceIndex(
                    0, false, false, false);

        QApplication::restoreOverrideCursor();
        return;
    }
    qDebug("MW: slotLayoutNodeSizesByOutDegree() setting size");
    nodeSizesByOutDegreeAct->setChecked(true);
    nodeSizesByOutDegreeBx->setChecked(true);
    nodeSizesByInDegreeAct->setChecked(false);
    nodeSizesByInDegreeBx->setChecked(false);

    askAboutWeights();

    statusMessage( tr("Embedding node size model on the network.... ")  );
    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

    activeGraph.layoutVerticesSizeByProminenceIndex(
                1,considerWeights,inverseWeights,
                filterIsolateNodesAct->isChecked());

    QApplication::restoreOverrideCursor( );
}



/**
 * @brief MainWindow::slotLayoutNodeSizesByInDegree
 * Resizes all nodes according to their inDegree
 * Called when user selects the relevant menu entry or the option in the toolbox
 * @param checked
 */
void MainWindow::slotLayoutNodeSizesByInDegree(bool checked){
    if (!fileLoaded && !networkModified  )  {
        QMessageBox::critical(this, "Error",tr("You must be dreaming! \nLoad a network file or create a new network first. \nThen we can talk about layouts!"), "OK",0);
        statusMessage( tr("I am really sorry. You must really load a file first... ")  );
        return;
    }

    qDebug("MW: slotLayoutNodeSizesByInDegree()");

    if (checked != true) {
        qDebug("MW: slotLayoutNodeSizesByInDegree() resetting size");
        nodeSizesByInDegreeAct->setChecked(false);
        nodeSizesByInDegreeBx->setChecked(false);

        QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

        activeGraph.layoutVerticesSizeByProminenceIndex(
                    0, false,false, false);

        QApplication::restoreOverrideCursor();
        return;
    }
    qDebug("MW: slotLayoutNodeSizesByInDegree() setting size");
    nodeSizesByOutDegreeAct->setChecked(false);
    nodeSizesByOutDegreeBx->setChecked(false);
    nodeSizesByInDegreeAct->setChecked(true);
    nodeSizesByInDegreeBx->setChecked(true);

    askAboutWeights();

    statusMessage( tr("Embedding node size model on the network.... ")  );
    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

    activeGraph.layoutVerticesSizeByProminenceIndex(
                9, considerWeights, inverseWeights,
                filterIsolateNodesAct->isChecked());

    QApplication::restoreOverrideCursor( );

}


/**
 * @brief MainWindow::slotLayoutGuides
 * @param state
 */
void MainWindow::slotLayoutGuides(int state){
    qDebug()<< "MW:slotLayoutGuides()";
    if (!fileLoaded && !networkModified  )  {
        QMessageBox::critical(this, "Error",tr("There are node nodes yet!\nLoad a network file or create a new network first. \nThen we can talk about layouts!"), "OK",0);
        statusMessage( tr("I am really sorry. You must really load a file first... ")  );
        //layoutGuidesBx->setCheckState(Qt::Unchecked);
        return;
    }

    if (state){
        qDebug()<< "MW:slotLayoutGuides() - will be displayed";
        statusMessage( tr("Layout Guides will be displayed") );
    }
    else {
        qDebug()<< "MW:slotLayoutGuides() - will NOT be displayed";
        graphicsWidget->clearGuides();
        statusMessage( tr("Layout Guides will not be displayed") );
    }
}


/**
 * @brief MainWindow::slotLayoutCircularByProminenceIndex
 * Checks sender text() to find out who QMenu item was pressed
 * calls slotLayoutCircularByProminenceIndex(QString)
 */
void MainWindow::slotLayoutCircularByProminenceIndex(){
    qDebug() << "MainWindow::slotLayoutCircularByProminenceIndex()";
    if (!fileLoaded && !networkModified  )  {
                QMessageBox::critical(
                    this, "Error",
                    tr("Sorry, I can't follow! "
                       "\nLoad a network file or create a new network first. \n"
                       "Then we can talk about layouts!"), "OK",0);
        statusMessage(  QString(tr("Nothing to layout! Are you dreaming?"))  );
        return;
    }
    QAction *menuitem=(QAction *) sender();
    QString menuItemText=menuitem->text();
    qDebug() << "MainWindow::slotLayoutCircularByProminenceIndex() - " <<
                "SENDER MENU IS " << menuItemText;

    slotLayoutCircularByProminenceIndex(menuItemText);

}




/**
 * @brief MainWindow::slotLayoutCircularByProminenceIndex
 * Overloaded - called when selectbox changes in the toolbox
 * or from slotLayoutCircularByProminenceIndex() when the user click on menu
 * Repositions all nodes  on a Circular layout based on that index
*  More prominent nodes are closer to the centre of the screen.
 */
void MainWindow::slotLayoutCircularByProminenceIndex(QString choice=""){
        qDebug() << "MainWindow::slotLayoutCircularByProminenceIndex() ";
    if (!fileLoaded && !networkModified  )  {
                QMessageBox::critical(
                    this, "Error",
                    tr("Sorry, I can't follow! "
                       "\nLoad a network file or create a new network first. \n"
                       "Then we can talk about layouts!"), "OK",0);
        statusMessage(  QString(tr("Nothing to layout! Are you dreaming?"))  );
        return;
    }
    int userChoice = 0;
    QString prominenceIndexName = choice;

    if ( prominenceIndexName.contains("Degree Centrality") )
        userChoice=1;
    else if ( prominenceIndexName == "Closeness Centrality")
        userChoice=2;
    else if ( prominenceIndexName.contains("Influence Range Closeness Centrality"))
        userChoice=3;
    else if ( prominenceIndexName.contains("Betweenness Centrality"))
        userChoice=4;
    else if (prominenceIndexName.contains("Stress Centrality"))
        userChoice=5;
    else if (prominenceIndexName.contains("Eccentricity Centrality"))
        userChoice=6;
    else if (prominenceIndexName.contains("Power Centrality"))
        userChoice=7;
    else if (prominenceIndexName.contains("Information Centrality"))
        userChoice=8;
    else if (prominenceIndexName.contains("Degree Prestige"))
        userChoice=9;
    else if (prominenceIndexName.contains("PageRank Prestige"))
        userChoice=10;
    else if (prominenceIndexName.contains("Proximity Prestige"))
        userChoice=11;

    qDebug() << "MainWindow::slotLayoutCircularByProminenceIndex() "
             << "prominenceIndexName " << prominenceIndexName
                << " userChoice " << userChoice;

    toolBoxLayoutByIndexSelect->setCurrentIndex(userChoice+1);
    toolBoxLayoutByIndexTypeSelect->setCurrentIndex(0);

    bool dropIsolates=false;
    //check if CC was selected and the graph is disconnected.
    if (userChoice == 2 ) {
        int connectedness=activeGraph.connectedness();
        switch ( connectedness ) {
        case 1:
            break;
        case 2:
            break;
        case -1:
            QMessageBox::information(this,
                                  "Closeness Centrality",
                                     tr(
                                         "Undirected graph has isolate nodes!\n"
                                         "Since this network has isolate nodes, "
                                         "I will drop them from calculations "
                                         "otherwise the CC index "
                                         "cannot be computed, because d(u,v) will be "
                                         "infinite for any isolate node u or v.\n"
                                         "You can also try the slightly different "
                                         "but improved Influence Range Closeness index "
                                         "which considers how proximate is each node "
                                         "to the nodes in its influence range.\n"
                                         "Read more in the SocNetV manual."
                                         ), "OK",0);
            dropIsolates=true;
            break;

        case -3:
            QMessageBox::information(this,
                                  "Closeness Centrality",
                                  tr(
                                     "Directed graph has isolate nodes!\n"
                                     "Since this digraph has isolate nodes, "
                                     "I will drop them from calculations"
                                      "otherwise Closeness Centrality "
                                     "index can not be defined, because d(u,v) will be "
                                     "infinite for any isolate node u or v.\n"
                                     "You can conside using the slightly different "
                                     "but improved Influence Range Closeness index "
                                     "which considers how proximate is each node "
                                     "to the nodes in its influence range.\n"
                                      "Read more in the SocNetV manual."
                                     ), "OK",0);
            dropIsolates=true;
            break;
        default:
            QMessageBox::critical(this,
                                  "Centrality Closeness",
                                  tr(
                                      "Disconnected graph/digraph!\n"
                                      "Since this network is disconnected, "
                                      "the ordinary Closeness Centrality "
                                      "index is not defined, because d(u,v) will be "
                                      "infinite for any isolate nodes u or v.\n"
                                      "Please use the slightly different but improved "
                                      "Influence Range Closeness (IRCC) index "
                                      "which considers how proximate is each node "
                                      "to the nodes in its influence range.\n"
                                      "Read more in the SocNetV manual."
                                      ), "OK",0);
            return;
            break;
        };

    }
    if (userChoice==8 && activeNodes() > 200) {
        switch(
               QMessageBox::critical(
                   this, "Slow function warning",
                   tr("Please note that this function is <b>VERY SLOW</b> on large "
                      "networks (n>200), since it will calculate  a (n x n) matrix A with:"
                      "Aii=1+weighted_degree_ni"
                      "Aij=1 if (i,j)=0"
                      "Aij=1-wij if (i,j)=wij"
                      "Next, it will compute the inverse matrix C of A."
                      "The computation of the inverse matrix is VERY CPU intensive function."
                      "because it uses the Gauss-Jordan elimination algorithm.\n\n "
                      "Are you sure you want to continue?"), QMessageBox::Ok|QMessageBox::Cancel,QMessageBox::Cancel) ) {
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

    askAboutWeights();

    double x0=scene->width()/2.0;
    double y0=scene->height()/2.0;
    double maxRadius=(graphicsWidget->height()/2.0)-50;          //pixels
    statusMessage(  QString(tr("Calculating new nodes positions. Please wait...")) );
    graphicsWidget->clearGuides();
    createProgressBar();
    activeGraph.layoutCircularByProminenceIndex(
                x0, y0, maxRadius, userChoice,
                considerWeights, inverseWeights,
                filterIsolateNodesAct->isChecked() || dropIsolates);
    destroyProgressBar();
    statusMessage( tr("Nodes in inner circles have greater prominence index.") );
}





/**
 * @brief MainWindow::slotLayoutNodeSizesByProminenceIndex
 * Called when selectbox changes in the toolbox
 */
void MainWindow::slotLayoutNodeSizesByProminenceIndex(QString choice=""){
        qDebug() << "MainWindow::slotLayoutNodeSizesByProminenceIndex() ";
    if (!fileLoaded && !networkModified  )  {
                QMessageBox::critical(
                    this, "Error",
                    tr("Sorry, I can't follow! "
                       "\nLoad a network file or create a new network first. \n"
                       "Then we can talk about layouts!"), "OK",0);
        statusMessage(  QString(tr("Nothing to layout! Are you dreaming?"))  );
        return;
    }
    int userChoice = 0;
    QString prominenceIndexName = choice;

    if ( prominenceIndexName.contains("Degree Centrality") )
        userChoice=1;
    else if ( prominenceIndexName == "Closeness Centrality")
        userChoice=2;
    else if ( prominenceIndexName.contains("Influence Range Closeness Centrality"))
        userChoice=3;
    else if ( prominenceIndexName.contains("Betweenness Centrality"))
        userChoice=4;
    else if (prominenceIndexName.contains("Stress Centrality"))
        userChoice=5;
    else if (prominenceIndexName.contains("Eccentricity Centrality"))
        userChoice=6;
    else if (prominenceIndexName.contains("Power Centrality"))
        userChoice=7;
    else if (prominenceIndexName.contains("Information Centrality"))
        userChoice=8;
    else if (prominenceIndexName.contains("Degree Prestige"))
        userChoice=9;
    else if (prominenceIndexName.contains("PageRank Prestige"))
        userChoice=10;
    else if (prominenceIndexName.contains("Proximity Prestige"))
        userChoice=11;

    qDebug() << "MainWindow::slotLayoutNodeSizesByProminenceIndex() "
             << "prominenceIndexName " << prominenceIndexName
                << " userChoice " << userChoice;

    toolBoxLayoutByIndexSelect->setCurrentIndex(userChoice+1);
    toolBoxLayoutByIndexTypeSelect->setCurrentIndex(0);

    //check if CC was selected and the graph is disconnected.
    bool dropIsolates=false;
    if (userChoice == 2 ) {
        int connectedness=activeGraph.connectedness();
        switch ( connectedness ) {
        case 1:
            break;
        case 2:
            break;
        case -1:
            QMessageBox::information(this,
                                  "Closeness Centrality",
                                     tr(
                                         "Undirected graph has isolate nodes!\n"
                                         "Since this network has isolate nodes, "
                                         "I will drop them from calculations "
                                         "otherwise the CC index "
                                         "cannot be computed, because d(u,v) will be "
                                         "infinite for any isolate node u or v.\n"
                                         "You can also try the slightly different "
                                         "but improved Influence Range Closeness index "
                                         "which considers how proximate is each node "
                                         "to the nodes in its influence range.\n"
                                         "Read more in the SocNetV manual."
                                         ), "OK",0);
            dropIsolates=true;
            break;

        case -3:
            QMessageBox::information(this,
                                  "Closeness Centrality",
                                  tr(
                                     "Directed graph has isolate nodes!\n"
                                     "Since this digraph has isolate nodes, "
                                         "I will drop them from calculations "
                                         "otherwise the CC index "
                                         "cannot be computed, because d(u,v) will be "
                                         "infinite for any isolate node u or v.\n"
                                         "You can also try the slightly different "
                                         "but improved Influence Range Closeness index "
                                         "which considers how proximate is each node "
                                         "to the nodes in its influence range.\n"
                                         "Read more in the SocNetV manual."
                                     ), "OK",0);
            dropIsolates=true;
            break;
        default:
            QMessageBox::critical(this,
                                  "Centrality Closeness",
                                  tr(
                                      "Disconnected graph/digraph!\n"
                                      "Since this network is disconnected, "
                                      "the ordinary Closeness Centrality "
                                      "index is not defined, because d(u,v) will be "
                                      "infinite for any isolate nodes u or v.\n"
                                      "Please use the slightly different but improved "
                                      "Influence Range Closeness (IRCC) index "
                                      "which considers how proximate is each node "
                                      "to the nodes in its influence range.\n"
                                      "Read more in the SocNetV manual."
                                      ), "OK",0);
            return;
            break;
        };

    }
    if (userChoice==8 && activeNodes() > 200) {
        switch(
               QMessageBox::critical(
                   this, "Slow function warning",
                   tr("Please note that this function is <b>VERY SLOW</b> on large "
                      "networks (n>200), since it will calculate  a (n x n) matrix A with:"
                      "Aii=1+weighted_degree_ni"
                      "Aij=1 if (i,j)=0"
                      "Aij=1-wij if (i,j)=wij"
                      "Next, it will compute the inverse matrix C of A."
                      "The computation of the inverse matrix is VERY CPU intensive function."
                      "because it uses the Gauss-Jordan elimination algorithm.\n\n "
                      "Are you sure you want to continue?"), QMessageBox::Ok|QMessageBox::Cancel,QMessageBox::Cancel) ) {
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

    askAboutWeights();

    statusMessage(  QString(tr("Calculating new node sizes. Please wait...")) );
    graphicsWidget->clearGuides();
    createProgressBar();
    activeGraph.layoutVerticesSizeByProminenceIndex(
                userChoice, considerWeights,
                inverseWeights, filterIsolateNodesAct->isChecked() || dropIsolates);
    destroyProgressBar();
    statusMessage( tr("Bigger nodes have greater prominence index.") );
}



/**
 * @brief MainWindow::slotLayoutLevelByProminenceIndex
 * Checks sender text() to find out who QMenu item was pressed
 * and what prominence index was chosen
 * calls slotLayoutLevelByProminenceIndex(QString)
  */
void MainWindow::slotLayoutLevelByProminenceIndex(){
    if (!fileLoaded && !networkModified  )  {
        QMessageBox::critical
                (this,
                 "Error",
                 tr("Sorry, I can't follow! "
                    "\nLoad a network file or create a new network first. "
                    "\nThen we can talk about layouts!"
                    ),
                 "OK",0
                 );
        statusMessage(  QString(tr("Nothing to layout! Are you dreaming?"))  );
        return;
    }
    QAction *menuitem=(QAction *) sender();
    QString menuItemText = menuitem->text();
    qDebug() << "MainWindow::slotLayoutLevelByProminenceIndex() - " <<
                "SENDER MENU IS " << menuItemText;

    slotLayoutLevelByProminenceIndex(menuItemText);

}




/**
 * @brief MainWindow::slotLayoutLevelByProminenceIndex(QString)
 * Overloaded - called when user clicks on toolbox options and when
 * the user selects a menu option (called by slotLayoutLevelByProminenceIndex())
 * Repositions all nodes  on different top-down levels according to the
*  chosen prominence index.
* More prominent nodes are closer to the top of the canvas
 */
void MainWindow::slotLayoutLevelByProminenceIndex(QString choice=""){
    if (!fileLoaded && !networkModified  )  {
        QMessageBox::critical
                (this,
                 "Error",
                 tr("Sorry, I can't follow! "
                    "\nLoad a network file or create a new network first. "
                    "\nThen we can talk about layouts!"
                    ),
                 "OK",0
                 );
        statusMessage(  QString(tr("Nothing to layout! Are you dreaming?"))  );
        return;
    }
    int userChoice = 0;
    QString prominenceIndexName = choice;

    if (prominenceIndexName == "Degree Centrality")
        userChoice=1;
    else if (prominenceIndexName == "Closeness Centrality")
        userChoice=2;
    else if (prominenceIndexName == "Influence Range Closeness Centrality")
        userChoice=3;
    else if (prominenceIndexName == "Betweenness Centrality")
        userChoice=4;
    else if (prominenceIndexName == "Stress Centrality")
        userChoice=5;
    else if (prominenceIndexName == "Eccentricity Centrality")
        userChoice=6;
    else if (prominenceIndexName == "Power Centrality")
        userChoice=7;
    else if (prominenceIndexName ==  "Information Centrality")
        userChoice=8;
    else if (prominenceIndexName == "Degree Prestige")
        userChoice=9;
    else if (prominenceIndexName ==  "PageRank Prestige")
        userChoice=10;
    else if (prominenceIndexName ==  "Proximity Prestige")
        userChoice=11;

    qDebug() << "MainWindow::slotLayoutLevelByProminenceIndex() "
             << "prominenceIndexName " << prominenceIndexName
              << " userChoice " << userChoice;

    toolBoxLayoutByIndexSelect->setCurrentIndex(userChoice+1);
    toolBoxLayoutByIndexTypeSelect->setCurrentIndex(1);

    bool dropIsolates=false;
    //check if CC was selected and the graph is disconnected.
    if (userChoice == 2 ) {
        int connectedness=activeGraph.connectedness();
        switch ( connectedness ) {
        case 1:
            break;
        case 2:
            break;
        case -1:
            QMessageBox::information(this,
                                  "Closeness Centrality",
                                  tr(
                                         "Undirected graph has isolate nodes!\n"
                                         "Since this network has isolate nodes, "
                                         "I will drop them from calculations "
                                         "otherwise the CC index "
                                         "cannot be computed, because d(u,v) will be "
                                         "infinite for any isolate node u or v.\n"
                                         "You can also try the slightly different "
                                         "but improved Influence Range Closeness index "
                                         "which considers how proximate is each node "
                                         "to the nodes in its influence range.\n"
                                          "Read more in the SocNetV manual."
                                     ), "OK",0);
            dropIsolates=true;
            break;

        case -3:
            QMessageBox::information(this,
                                  "Closeness Centrality",
                                  tr(
                                     "Directed graph has isolate nodes!\n"
                                     "Since this digraph has isolate nodes, "
                                         "I will drop them from calculations "
                                         "otherwise the CC index "
                                         "cannot be computed, because d(u,v) will be "
                                         "infinite for any isolate node u or v.\n"
                                         "You can also try the slightly different "
                                         "but improved Influence Range Closeness index "
                                         "which considers how proximate is each node "
                                         "to the nodes in its influence range.\n"
                                         "Read more in the SocNetV manual."
                                     ), "OK",0);
            dropIsolates=true;
            break;
        default:
            QMessageBox::critical(this,
                                  "Centrality Closeness",
                                  tr(
                                      "Disconnected graph/digraph!\n"
                                      "Since this network is disconnected, "
                                      "the ordinary Closeness Centrality "
                                      "index is not defined, because d(u,v) will be "
                                      "infinite for any isolate nodes u or v.\n"
                                      "Please use the slightly different but improved "
                                      "Influence Range Closeness (IRCC) index "
                                      "which considers how proximate is each node "
                                      "to the nodes in its influence range.\n"
                                      "Read more in the SocNetV manual."
                                      ), "OK",0);
            return;
            break;
        };

    }
    if (userChoice==8 && activeNodes() > 200) {
        switch(
               QMessageBox::critical(
                   this, "Slow function warning",
                   tr("Please note that this function is <b>VERY SLOW</b> on large "
                      "networks (n>200), since it will calculate  a (n x n) matrix A with:"
                      "Aii=1+weighted_degree_ni"
                      "Aij=1 if (i,j)=0"
                      "Aij=1-wij if (i,j)=wij"
                      "Next, it will compute the inverse matrix C of A."
                      "The computation of the inverse matrix is VERY CPU intensive function."
                      "because it uses the Gauss-Jordan elimination algorithm.\n\n "
                      "Are you sure you want to continue?"), QMessageBox::Ok|QMessageBox::Cancel,QMessageBox::Cancel) ) {
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

    askAboutWeights();
    double maxWidth=scene->width();
    double maxHeight=scene->height(); //pixels
    statusMessage(  QString(tr("Calculating new nodes positions. Please wait...")) );
    graphicsWidget->clearGuides();
    createProgressBar();
    activeGraph.layoutLevelByProminenceIndex(
                maxWidth, maxHeight, userChoice,
                considerWeights, inverseWeights,
                filterIsolateNodesAct->isChecked() || dropIsolates);
    destroyProgressBar();
    statusMessage( tr("Nodes in upper levels are more prominent. ") );
    }


/**
*	Returns the amount of active edges on the scene.
*/
int MainWindow::activeEdges(){
    qDebug () << "MW::activeEdges()";
    return activeGraph.totalEdges();
}





/**
*	Returns the number of active nodes on the scene.
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
    QString fn = dataDir + "socnetv-report-invert-adjacency-matrix.dat";

    activeGraph.writeInvertAdjacencyMatrix(fn, networkName.toLocal8Bit()) ;

    //Open a text editor window for the new file created by graph class
    TextEditor *ed = new TextEditor(fn);
    tempFileNameNoPath=fn.split( "/");
    ed->setWindowTitle(tr("View Adjacency Matrix - ") + tempFileNameNoPath.last());
    ed->show();

}




void MainWindow::askAboutWeights(){
    if (!activeGraph.isWeighted()  ){
        considerWeights=false;
        return;
    }
    if (askedAboutWeights)
        return;

    if ( ! considerEdgeWeightsAct->isChecked() && !considerWeights){
        switch( QMessageBox::information(
                    this, "Edge weights and Distances",
                    tr("This network is weighted.\n"
                       "Take edge weights into account (Default: No)?"),
                    QMessageBox::Yes|QMessageBox::No, QMessageBox::No) )
        {
        case QMessageBox::Yes:
            considerWeights=true;
            considerEdgeWeightsAct->setChecked(true);
            break;
        case QMessageBox::No:
            considerWeights=false;
            considerEdgeWeightsAct->setChecked(false);
            break;
        default: // just for sanity
            considerWeights=false;
            considerEdgeWeightsAct->setChecked(false);
            return;
            break;
        }

    }

    if (considerWeights){
        switch( QMessageBox::information
                ( this, "Edge weights and Distances",
                  tr("Inverse edge weights during calculations? (Default: Yes)?\n\n"
                     "If the weights denote cost (i.e. ), press No, since the "
                     "distance between two nodes should be the quickest or cheaper one. \n\n"
                     "If the weights denote value or strength (i.e. votes or interaction), "
                     "press Yes to inverse the weights, since the distance between two "
                     "nodes should be the most valuable one."),
                  QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes) )
        {
        case QMessageBox::Yes:
            inverseWeights=true;
            break;
        case QMessageBox::No:
            inverseWeights=false;
            break;
        default: // just for sanity
            inverseWeights=true;
            return;
            break;
        }
    }
    askedAboutWeights=true;
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
            if ( min>jim->nodeNumber() && jim->isEnabled() ) min=jim->nodeNumber();
            if ( max<jim->nodeNumber() && jim->isEnabled() ) max=jim->nodeNumber();
        }
    }
    i=QInputDialog::getInt(this, tr("Distance between two nodes"),
                           tr("Select source node:  ("
                              +QString::number(min).toLatin1()
                              +"..."+QString::number(max).toLatin1()
                              +"):"), min, 1, max , 1, &ok )   ;
    if (!ok) {
        statusMessage( "Distance calculation operation cancelled." );
        return;
    }

    j=QInputDialog::getInt(this, tr("Distance between two nodes"),
                           tr("Select target node:  ("
                              +QString::number(min).toLatin1()+"..."
                              +QString::number(max).toLatin1()
                              +"):"),min, 1, max , 1, &ok )   ;
    if (!ok) {
        statusMessage( tr("Distance calculation operation cancelled.") );
        return;
    }

    qDebug() << "source " << i  << " target" <<  j;

    if (activeGraph.isSymmetric() && i>j) {
        qSwap(i,j);
    }


    askAboutWeights();

     int distance = activeGraph.distance(i,j,
                                         considerWeights,
                                         inverseWeights);
    if ( distance > 0 && distance < RAND_MAX)
        QMessageBox::information(this, tr("Distance"), tr("Network distance (")
                                 +QString::number(i)+", "+QString::number(j)
                                 +") = "+QString::number(distance)
                                 +tr("\nThe nodes are connected."),"OK",0);
    else
        QMessageBox::information(this, tr("Distance"), tr("Network distance (")
                                 +QString::number(i)+", "+QString::number(j)
                                 +") = "+ QString("\xE2\x88\x9E")
                                 +tr("\nThe nodes are not connected."),"OK",0);
}




/**
*  Invokes calculation of the matrix of geodesic distances for the loaded network, then displays it.
*/
void MainWindow::slotDistancesMatrix(){
    qDebug("MW: slotDistancesMatrix()");
    if (!fileLoaded && !networkModified  )  {
        QMessageBox::critical(this, "Error",tr("There are no nodes nor edges!\nLoad a network file or create a new network. \nThen ask me to compute something!"), "OK",0);
        statusMessage(  QString(tr("Nothing to do!"))  );
        return;
    }
    statusMessage( tr("Creating distance matrix. Please wait...") );
    QString fn = dataDir + "socnetv-report-distance-matrix.dat";


    askAboutWeights();

    createProgressBar();
    activeGraph.writeDistanceMatrix(fn, networkName.toLocal8Bit(),
                                    considerWeights, inverseWeights,
                                    filterIsolateNodesAct->isChecked());

    destroyProgressBar();

    //Open a text editor window for the new file created by graph class
    TextEditor *ed = new TextEditor(fn);
    tempFileNameNoPath=fn.split( "/");

    ed->setWindowTitle(tempFileNameNoPath.last());
    ed->show();
    QApplication::restoreOverrideCursor();
    statusMessage(tr("Distance matrix saved as: ")+tempFileNameNoPath.last());
}




/**
*  Invokes calculation of the sigmas matrix (the number of geodesic paths between each pair of nodes in the loaded network), then displays it.
*/
void MainWindow::slotGeodesicsMatrix(){
    qDebug("MW: slotViewNumberOfGeodesics()");
    if (!fileLoaded && !networkModified  )  {
        QMessageBox::critical(this, "Error",tr("There are no nodes nor edges!\nLoad a network file or create a new network. \nThen ask me to compute something!"), "OK",0);
        statusMessage(  QString(tr("Nothing to do!"))  );
        return;
    }

    QString fn = dataDir + "socnetv-report-sigmas-matrix.dat";

    askAboutWeights();

    statusMessage( tr("Creating number of geodesics matrix. Please wait...") );
    createProgressBar();

    activeGraph.writeNumberOfGeodesicsMatrix(fn, networkName.toLocal8Bit(),
                                             considerWeights, inverseWeights);

    destroyProgressBar();

    //Open a text editor window for the new file created by graph class

    TextEditor *ed = new TextEditor(fn);
    tempFileNameNoPath=fn.split( "/");
    ed->setWindowTitle(tempFileNameNoPath.last());
    ed->show();
    QApplication::restoreOverrideCursor();
    statusMessage(tr("Matrix of geodesic path counts saved as: ")
                  + tempFileNameNoPath.last());
}



/**  Displays the network diameter (largest geodesic) */
void MainWindow::slotDiameter() {
    if (!fileLoaded && !networkModified  )  {
        QMessageBox::critical(this, "Error"
                              ,tr("There are no nodes nor edges!\n"
                                  "Load a network file or create a new network. \n"
                                  "Then ask me to compute something!"), "OK",0);

        statusMessage(  QString(tr("Cannot find the diameter of nothing..."))  );
        return;
    }


    askAboutWeights();

    createProgressBar();

    int netDiameter=activeGraph.diameter(considerWeights, inverseWeights);

    destroyProgressBar();

    if ( activeGraph.isWeighted() && considerWeights )
        QMessageBox::information(this, "Diameter",
                                 tr("Diameter =  ")
                                 + QString::number(netDiameter) +
                                 tr("\n\nSince this is a weighted network \n"
                                 "the diameter can be more than N"),
                                 "OK",0);
    else if ( activeGraph.isWeighted() && !considerWeights )
        QMessageBox::information(this, "Diameter",
                                 tr("Diameter =  ")
                                 + QString::number(netDiameter) +
                                 tr("\n\nThis is the diameter of the \n"
                                    "corresponding network without weights"),
                                 "OK",0);
    else
        QMessageBox::information(this, "Diameter",
                                 tr("Diameter =  ")
                                 + QString::number(netDiameter) +
                                 tr("\n\nSince this is a non-weighted network, \n"
                                 "the diameter is always less than N-1."),
                                 "OK",0);
    statusMessage( tr("Diameter calculated. Ready.") );

}





/**  Displays the  average shortest path length (average graph distance) */
void MainWindow::slotAverageGraphDistance() {
    if (!fileLoaded && !networkModified  )  {
        QMessageBox::critical(this, "Error",tr("There are no nodes nor edges!\nLoad a network file or create a new network. \nThen ask me to compute something!"), "OK",0);

        statusMessage(  QString(tr("Cannot find the diameter of nothing..."))  );
        return;
    }

    askAboutWeights();

    createProgressBar();

    float averGraphDistance=activeGraph.averageGraphDistance(
                considerWeights, inverseWeights);

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
    QString fn = dataDir + "socnetv-report-eccentricity.dat";

    askAboutWeights();
    statusMessage(  QString(tr(" Please wait...")));

    createProgressBar();
    activeGraph.writeEccentricity(
                fn, considerWeights, inverseWeights,
                filterIsolateNodesAct->isChecked());
    destroyProgressBar();

    TextEditor *ed = new TextEditor(fn);        //OPEN A TEXT EDITOR WINDOW
    tempFileNameNoPath=fn.split( "/");
    ed->setWindowTitle(tempFileNameNoPath.last());
    ed->show();
    QApplication::restoreOverrideCursor();
    statusMessage(tr("Eccentricity report saved as: ") + tempFileNameNoPath.last());
}





/**
 * @brief MainWindow::slotConnectedness
 */
void MainWindow::slotConnectedness(){
    if (!fileLoaded && !networkModified  )  {
        QMessageBox::critical(this, "Error",tr("There are no nodes nor edges!\nLoad a network file or create a new network. \nThen ask me to compute something!"), "OK",0);

        statusMessage(  QString(tr("Nothing to do..."))  );
        return;
    }

    createProgressBar();

    int connectedness=activeGraph.connectedness();

    qDebug () << "MW::connectedness result " << connectedness;

    destroyProgressBar();

    switch ( connectedness ) {
    case 1:
        QMessageBox::information(this, "Connectedness", "This undirected graph "
                                 "is connected.", "OK",0);
        break;
    case 0:
        QMessageBox::information(this, "Connectedness", tr("This undirected graph "
                                 " is not connected."), "OK",0);
        break;
    case 2:
        QMessageBox::information(this, "Connectedness", tr("This directed graph "
                                 "is strongly connected."), "OK",0);
        break;
    case -1:
        QMessageBox::information(this, "Connectedness", tr("This undirected graph "
                                 "is disconnected because isolate nodes exist. \n"
                                 "It can become connected by dropping isolates."), "OK",0);
        break;
    case -2:
        QMessageBox::information(this, "Connectedness", tr("This directed graph "
                                 "is unilaterally connected. \n"
                                                           "For every pair of "
                                 "nodes (u,v) there is a path either from u to v or "
                                 "from v to u, but not always both."), "OK",0);
        break;

    case -3:
        QMessageBox::information(this, "Connectedness", "This directed graph "
                                 "is disconnected because isolate nodes exist. \n"
                                 "It can become strongly connected by dropping isolates.", "OK",0);
        break;
    case -4:
        QMessageBox::information(this, "Connectedness", "This directed graph "
                                 "is disconnected. \nThere are pairs of nodes that "
                                 "are disconnected.", "OK",0);
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
void MainWindow::slotWalksOfGivenLength(){
    if (!fileLoaded && !networkModified  )  {
        QMessageBox::critical(this, "Error",tr("Nothing to do! \nLoad a network file or create a new network. \nThen ask me to compute something!"), "OK",0);
        statusMessage(  QString(tr(" No network here. Sorry. Nothing to do."))  );
        return;
    }

    QString fn = dataDir + "socnetv-report-number-of-walks.dat";
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
    ed->setWindowTitle(tempFileNameNoPath.last());
    ed->show();
    QApplication::restoreOverrideCursor();
    statusMessage(tr("Number of walks saved as: ") + tempFileNameNoPath.last());
}



/**
*	Calls Graph:: writeTotalNumberOfWalksMatrix() to calculate and print
*   the total number of walks of any length , between each pair of nodes.
*/
void MainWindow::slotTotalWalks(){
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
    QString fn = dataDir + "socnetv-report-total-number-of-walks.dat";
    createProgressBar();

    int maxLength=activeNodes()-1;
    activeGraph.writeTotalNumberOfWalksMatrix(fn, networkName, maxLength);

    destroyProgressBar();

    TextEditor *ed = new TextEditor(fn);        //OPEN A TEXT EDITOR WINDOW
    tempFileNameNoPath=fn.split( "/");
    ed->setWindowTitle( tempFileNameNoPath.last());
    ed->show();
    QApplication::restoreOverrideCursor();
    statusMessage("Total number of walks saved as: " + tempFileNameNoPath.last());

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

    QString fn = dataDir + "socnetv-report-reachability-matrix.dat";

    createProgressBar();

    activeGraph.writeReachabilityMatrix(fn, networkName);

    destroyProgressBar();

    TextEditor *ed = new TextEditor(fn);        //OPEN A TEXT EDITOR WINDOW
    tempFileNameNoPath=fn.split( "/");
    ed->setWindowTitle(tempFileNameNoPath.last());
    ed->show();
    QApplication::restoreOverrideCursor();
    statusMessage("Reachability Matrix saved as: " + tempFileNameNoPath.last());
}

/**
*	Calls Graph:: writeCliqueCensus() to write the number of cliques (triangles)
*  of each vertex into a file, then displays it.
*/
void MainWindow::slotCliqueCensus(){
    if (!fileLoaded && !networkModified  )  {
        QMessageBox::critical(this, "Error",tr("Nothing to do! \nLoad a network file or create a new network. \nThen ask me to compute something!"), "OK",0);
        statusMessage(  QString(tr(" No network here. Sorry. Nothing to do."))  );
        return;
    }
    QString fn = dataDir + "socnetv-report-clique-census.dat";
    bool considerWeights=true;

    createProgressBar();

    activeGraph.writeCliqueCensus(fn, considerWeights);

    destroyProgressBar();

    TextEditor *ed = new TextEditor(fn);        //OPEN A TEXT EDITOR WINDOW
    tempFileNameNoPath=fn.split( "/");
    ed->setWindowTitle(tempFileNameNoPath.last());
    ed->show();
    QApplication::restoreOverrideCursor();
    statusMessage("Clique Census saved as: " + tempFileNameNoPath.last());
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
    QString fn = dataDir + "socnetv-report-clustering-coefficients.dat";
    bool considerWeights=true;

    createProgressBar();

    activeGraph.writeClusteringCoefficient(fn, considerWeights);

    destroyProgressBar();

    TextEditor *ed = new TextEditor(fn);        //OPEN A TEXT EDITOR WINDOW
    tempFileNameNoPath=fn.split( "/");
    ed->setWindowTitle(tempFileNameNoPath.last());
    ed->show();
    QApplication::restoreOverrideCursor();
    statusMessage("Clustering Coefficients saved as: " + tempFileNameNoPath.last());
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
    QString fn = dataDir + "socnetv-report-triad-census.dat";
    bool considerWeights=true;

    createProgressBar();

    activeGraph.writeTriadCensus(fn, considerWeights);

    destroyProgressBar();

    TextEditor *ed = new TextEditor(fn);        //OPEN A TEXT EDITOR WINDOW
    tempFileNameNoPath=fn.split( "/");
    ed->setWindowTitle(tempFileNameNoPath.last());
    ed->show();
    QApplication::restoreOverrideCursor();
    statusMessage("Triad Census saved as: " + tempFileNameNoPath.last());
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
    QString fn = dataDir + "socnetv-report-centrality-out-degree.dat";

    createProgressBar();

    activeGraph.writeCentralityDegree(fn, considerWeights,
                                      filterIsolateNodesAct->isChecked() );

    destroyProgressBar();

    TextEditor *ed = new TextEditor(fn);        //OPEN A TEXT EDITOR WINDOW
    tempFileNameNoPath=fn.split( "/");
    ed->setWindowTitle(tempFileNameNoPath.last());
    ed->show();
    QApplication::restoreOverrideCursor();
    statusMessage(tr("Out-Degree Centralities saved as: ") + tempFileNameNoPath.last());
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
    bool dropIsolates=false;
    switch ( connectedness ) {
    case 1:
        break;
    case 2:
        break;
    case -1:
        QMessageBox::information(this,
                              "Closeness Centrality",
                              tr(
                                 "Undirected graph has isolate nodes!\n"
                                 "Since this network has isolate nodes, "
                                 "I will drop them from calculations "
                                 "otherwise the CC index "
                                 "cannot be computed, because d(u,v) will be "
                                 "infinite for any isolate node u or v.\n"
                                 "You can also try the slightly different "
                                 "but improved Influence Range Closeness index "
                                 "which considers how proximate is each node "
                                 "to the nodes in its influence range.\n"
                                  "Read more in the SocNetV manual."
                                 ), "OK",0);
        dropIsolates=true;
        break;

    case -3:
        QMessageBox::information(this,
                              "Closeness Centrality",
                              tr(
                                 "Directed graph has isolate nodes!\n"
                                 "Since this digraph has isolate nodes, "
                                     "I will drop them from calculations "
                                     "otherwise the CC index "
                                     "cannot be computed, because d(u,v) will be "
                                     "infinite for any isolate node u or v.\n"
                                     "You can also try the slightly different "
                                     "but improved Influence Range Closeness index "
                                     "which considers how proximate is each node "
                                     "to the nodes in its influence range.\n"
                                     "Read more in the SocNetV manual."
                                 ), "OK",0);
        dropIsolates=true;
        break;
    default:
        QMessageBox::critical(this,
                              "Centrality Closeness",
                              tr(
                                  "Disconnected graph/digraph!\n"
                                  "Since this network is disconnected, "
                                  "the ordinary Closeness Centrality "
                                  "index is not defined, because d(u,v) will be "
                                  "infinite for any isolate nodes u or v.\n"
                                  "Please use the slightly different but improved "
                                  "Influence Range Closeness (IRCC) index "
                                  "which considers how proximate is each node "
                                  "to the nodes in its influence range.\n"
                                  "Read more in the SocNetV manual."
                                  ), "OK",0);
        return;
        break;
    };


    askAboutWeights();

    QString fn = dataDir + "socnetv-report-centrality_closeness.dat";

    createProgressBar();

    activeGraph.writeCentralityCloseness(
                fn, considerWeights, inverseWeights,
                filterIsolateNodesAct->isChecked() || dropIsolates);

    destroyProgressBar();

    TextEditor *ed = new TextEditor(fn);        //OPEN A TEXT EDITOR WINDOW
    tempFileNameNoPath=fn.split( "/");
    ed->setWindowTitle( tempFileNameNoPath.last());
    ed->show();
    QApplication::restoreOverrideCursor();
    statusMessage(tr("Closeness Centralities  saved as: ") + tempFileNameNoPath.last());
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

    QString fn = dataDir + "socnetv-report-centrality_closeness_influence_range.dat";


    askAboutWeights();

    createProgressBar();

    activeGraph.writeCentralityClosenessInfluenceRange(
                fn, considerWeights,inverseWeights,
                filterIsolateNodesAct->isChecked());

    destroyProgressBar();

    statusMessage( QString(tr(" displaying file...")));

    TextEditor *ed = new TextEditor(fn);        //OPEN A TEXT EDITOR WINDOW
    tempFileNameNoPath=fn.split( "/");
    ed->setWindowTitle(tempFileNameNoPath.last());
    ed->show();
    QApplication::restoreOverrideCursor();
    statusMessage(tr("Influence Range Closeness Centrality saved as: ")+tempFileNameNoPath.last());
}




/**
*	Writes Betweenness Centralities into a file, then displays it.
*/
void MainWindow::slotCentralityBetweenness(){
    if (!fileLoaded && !networkModified  )  {
        QMessageBox::critical(this, "Error",tr("There are no nodes!\nLoad a network file or create a new network. \nThen ask me to compute something!"), "OK",0);

        statusMessage(  QString(tr(" Nothing to do..."))  );
        return;
    }
    QString fn = dataDir + "socnetv-report-centrality_betweenness.dat";


    askAboutWeights();

    statusMessage(  QString(tr(" Please wait...")));
    createProgressBar();
    activeGraph.writeCentralityBetweenness(
                fn, considerWeights, inverseWeights,
                filterIsolateNodesAct->isChecked());
    destroyProgressBar();

    statusMessage( QString(tr(" displaying file...")));

    TextEditor *ed = new TextEditor(fn);        //OPEN A TEXT EDITOR WINDOW
    tempFileNameNoPath=fn.split( "/");
    ed->setWindowTitle(tempFileNameNoPath.last() );
    ed->show();
    QApplication::restoreOverrideCursor();
    statusMessage(tr("Betweenness Centralities saved as: ")+tempFileNameNoPath.last());
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
        QMessageBox::warning(
                    this,
                    "Warning",
                    tr("Undirected graph!\n"
                       "Degree Prestige counts inbound edges, therefore is more "
                       "meaningful on directed graphs.\n"
                       "For undirected graphs, the DP scores are the same as "
                       "Degree Centrality..."), "OK",0);
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
    QString fn = dataDir + "socnetv-report-degree-prestige.dat";

    createProgressBar();

    activeGraph.writePrestigeDegree(fn, considerWeights,
                                    filterIsolateNodesAct->isChecked() );

    destroyProgressBar();

    TextEditor *ed = new TextEditor(fn);        //OPEN A TEXT EDITOR WINDOW
    tempFileNameNoPath=fn.split( "/");
    ed->setWindowTitle( tempFileNameNoPath.last());
    ed->show();
    QApplication::restoreOverrideCursor();
    statusMessage(tr("Degree Prestige (in-degree) saved as: ") + tempFileNameNoPath.last());
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
    QString fn = dataDir + "socnetv-report-prestige_pagerank.dat";


    askAboutWeights();

    statusMessage(  QString(tr(" Please wait...")));

    createProgressBar();
    activeGraph.writePrestigePageRank(fn, filterIsolateNodesAct->isChecked());
    destroyProgressBar();

    TextEditor *ed = new TextEditor(fn);        //OPEN A TEXT EDITOR WINDOW
    tempFileNameNoPath=fn.split( "/");
    ed->setWindowTitle(tempFileNameNoPath.last());
    ed->show();
    QApplication::restoreOverrideCursor();
    statusMessage(tr("PageRank Prestige indices saved as: ")+ tempFileNameNoPath.last());
}


/**
*	Writes Proximity Prestige indices into a file, then displays them.
*/
void MainWindow::slotPrestigeProximity(){
    if (!fileLoaded && !networkModified  )  {
        QMessageBox::critical(
                    this, "Error",
                    tr("There are no nodes!\n"
                       "Load a network file or create a new network. \n"
                       "Then ask me to compute something!"), "OK",0);

        statusMessage(  QString(tr(" Nothing to do..."))  );
        return;
    }
    QString fn = dataDir + "socnetv-report-centrality_proximity_prestige.dat";

    askAboutWeights();

    statusMessage(  QString(tr(" Please wait...")));
    createProgressBar();
    activeGraph.writePrestigeProximity(fn, true, false ,
                                       filterIsolateNodesAct->isChecked());
    destroyProgressBar();

    statusMessage( QString(tr(" displaying file...")));

    TextEditor *ed = new TextEditor(fn);        //OPEN A TEXT EDITOR WINDOW
    tempFileNameNoPath=fn.split( "/");
    ed->setWindowTitle( tempFileNameNoPath.last());
    ed->show();
    QApplication::restoreOverrideCursor();
    statusMessage(tr("Proximity Prestige Centralities saved as: ")+ tempFileNameNoPath.last());
}




/**
*	Writes Informational Centralities into a file, then displays it.	
*/
void MainWindow::slotCentralityInformation(){
    if (!fileLoaded && !networkModified  )  {
        QMessageBox::critical(
                    this,
                    "Error",
                    tr("There are no nodes!\n"
                       "Load a network file or create a new network. \n"
                       "Then ask me to compute something!"), "OK",0);
        statusMessage(  QString(tr(" Nothing to do..."))  );
        return;
    }
    if (activeNodes() > 200) {
        switch(
               QMessageBox::critical(
                   this, "Slow function warning",
                   tr("Please note that this function is <b>VERY SLOW</b> on large "
                      "networks (n>200), since it will calculate  a (n x n) matrix A with:"
                      "Aii=1+weighted_degree_ni"
                      "Aij=1 if (i,j)=0"
                      "Aij=1-wij if (i,j)=wij"
                      "Next, it will compute the inverse matrix C of A."
                      "The computation of the inverse matrix is VERY CPU intensive function."
                      "because it uses the Gauss-Jordan elimination algorithm.\n\n "
                      "Are you sure you want to continue?"), QMessageBox::Ok|QMessageBox::Cancel,QMessageBox::Cancel) ) {
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
    QString fn = dataDir + "socnetv-report-centrality_information.dat";
    statusMessage(  QString(tr(" Please wait...")));

    askAboutWeights();
    createProgressBar();
    activeGraph.writeCentralityInformation(fn,considerWeights, inverseWeights);
    destroyProgressBar();

    TextEditor *ed = new TextEditor(fn);
    tempFileNameNoPath=fn.split( "/");
    ed->setWindowTitle(tempFileNameNoPath.last());
    ed->show();
    QApplication::restoreOverrideCursor();
    statusMessage(tr("Information Centralities saved as: ")+ tempFileNameNoPath.last());
}




/**
*	Writes Stress Centralities into a file, then displays it.
*/
void MainWindow::slotCentralityStress(){
    if (!fileLoaded && !networkModified  )  {
        QMessageBox::critical(
                    this,
                    "Error",
                    tr("There are no nodes!\n"
                       "Load a network file or create a new network. \n"
                       "Then ask me to compute something!"), "OK",0);

        statusMessage(  QString(tr(" Nothing to do! Why don't you try creating something first?"))  );
        return;
    }
    QString fn = dataDir + "socnetv-report-centrality_stress.dat";


    askAboutWeights();

    statusMessage(  QString(tr(" Please wait...")));
    createProgressBar();
    activeGraph.writeCentralityStress(
                fn, considerWeights, inverseWeights,
                filterIsolateNodesAct->isChecked());
    destroyProgressBar();

    TextEditor *ed = new TextEditor(fn);        //OPEN A TEXT EDITOR WINDOW
    tempFileNameNoPath=fn.split( "/");
    ed->setWindowTitle(tempFileNameNoPath.last());
    ed->show();
    QApplication::restoreOverrideCursor();
    statusMessage(tr("Stress Centralities saved as: ")+ tempFileNameNoPath.last());
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
    QString fn = dataDir + "socnetv-report-centrality_power.dat";


    askAboutWeights();

    statusMessage(  QString(tr(" Please wait...")));
    createProgressBar();
    activeGraph.writeCentralityPower(
                fn, considerWeights, inverseWeights,
                filterIsolateNodesAct->isChecked());
    destroyProgressBar();

    TextEditor *ed = new TextEditor(fn);        //OPEN A TEXT EDITOR WINDOW
    tempFileNameNoPath=fn.split( "/");
    ed->setWindowTitle(tempFileNameNoPath.last());
    ed->show();
    QApplication::restoreOverrideCursor();
    statusMessage(tr("Stress Centralities saved as: ")+ tempFileNameNoPath.last());
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
    QString fn = dataDir + "socnetv-report-centrality_eccentricity.dat";


    askAboutWeights();

    statusMessage(  QString(tr(" Please wait...")));
    createProgressBar();
    activeGraph.writeCentralityEccentricity(
                fn, considerWeights, inverseWeights,
                filterIsolateNodesAct->isChecked());
    destroyProgressBar();

    TextEditor *ed = new TextEditor(fn);        //OPEN A TEXT EDITOR WINDOW
    tempFileNameNoPath=fn.split( "/");
    ed->setWindowTitle(tempFileNameNoPath.last());
    ed->show();
    QApplication::restoreOverrideCursor();
    statusMessage(tr("Eccentricity Centralities saved as: ")+ tempFileNameNoPath.last());
}



void MainWindow::createProgressBar(){

    if (showProgressBarAct->isChecked() || activeEdges() > 2000){
        progressDialog= new QProgressDialog("Please wait....", "Cancel", 0, activeGraph.vertices(), this);
        progressDialog -> setWindowModality(Qt::WindowModal);
        connect( &activeGraph, SIGNAL( updateProgressDialog(int) ), progressDialog, SLOT(setValue(int) ) ) ;
        progressDialog->setMinimumDuration(0);
    }

    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

}


void MainWindow::destroyProgressBar(){
    QApplication::restoreOverrideCursor();

    if (showProgressBarAct->isChecked() || activeEdges() > 1000)
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

    int newSize = QInputDialog::getInt(
                this,
                "Change node size",
                tr("Select new size for all nodes: (1-16)"),
                initNodeSize, 1, 16, 1, &ok );
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
    activeGraph.setAllVerticesSize(size);
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
    Turns on/off drawing edges as thick as their weights.
    TODO
*/
void MainWindow::slotDrawEdgesThickAsWeights() {

}



/**
*  Turns on/off displaying edge weight numbers
*/
void MainWindow::slotDisplayEdgesWeightNumbers(bool toggle) {
    if (!fileLoaded && ! networkModified) {
        QMessageBox::critical(this, "Error",tr("There are no edges! \nLoad a network file or create a new network first."), "OK",0);
        statusMessage( tr("No nodes or edges found. Sorry...") );
        return;
    }
    qDebug() << "MW::slotDisplayEdgesWeightNumbers - Toggling Edges Weights. Please wait...";
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
 * @brief MainWindow::slotConsiderEdgeWeights
 * @param toggle
 */
void MainWindow::slotConsiderEdgeWeights(bool toggle) {
   if (toggle) {
       considerWeights=true;
       askedAboutWeights=false;
        askAboutWeights(); // will only ask about inversion
   }
   else
       considerWeights=false;
}


/**
*  Turns on/off displaying edges
*/
void MainWindow::slotDisplayEdges(bool toggle){
    if (!fileLoaded && ! networkModified) {
        QMessageBox::critical(this, "Error",tr("There are no nodes nor edges! \nLoad a network file or create a new network first!"), "OK",0);

        statusMessage( tr("No edges found...") );
        return;
    }
    statusMessage( tr("Toggle Edges Arrows. Please wait...") );

    if (!toggle) 	{
        graphicsWidget->setAllItemsVisibility(TypeEdge, false);
        statusMessage( tr("Edges are invisible now. Click again the same menu to display them.") );
        return;
    }
    else{
        graphicsWidget->setAllItemsVisibility(TypeEdge, true);
        statusMessage( tr("Edges visible again...") );
    }

}



/**
*  Turns on/off the arrows of edges
*/
void MainWindow::slotDisplayEdgesArrows(bool toggle){
    if (!fileLoaded && ! networkModified) {
        QMessageBox::critical(this, "Error",tr("There are no edges! \nLoad a network file or create a new network first!"), "OK",0);

        statusMessage( tr("No edges found...") );
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
*  FIXME edges Bezier
*/
void MainWindow::slotDrawEdgesBezier(bool toggle){
    if (!fileLoaded && ! networkModified) {
        QMessageBox::critical(this, "Error",tr("There are no edges! \nLoad a network file or create a new network!"), "OK",0);

        statusMessage( tr("There are NO edges here!") );
        return;
    }
    statusMessage( tr("Toggle edges bezier. Please wait...") );
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
*  Changes the color of all edges
*/
void MainWindow::slotAllEdgesColor(){
    QColor color = QColorDialog::getColor( Qt::red, this,
                                           "Change the color of all nodes" );
    if (color.isValid()) {
        initNodeColor=color.name();
        QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
        qDebug() << "MainWindow::slotAllEdgesColor() : " << initNodeColor;
        //createProgressBar();
        activeGraph.setAllEdgesColor(initNodeColor);
        //destroyProgressBar();
        QApplication::restoreOverrideCursor();
        graphChanged();
        statusMessage( tr("Ready. ")  );
    }
    else {
        // user pressed Cancel
        statusMessage( tr("Nodes color change aborted. ") );
    }
}




/**
*  Changes the color of nodes' numbers
*/
void MainWindow::slotAllNumbersColor(){

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
        m_fileName = QFileDialog::getOpenFileName(
                    this, tr("Select one image"), getLastPath(),
                    tr("All (*);;PNG (*.png);;JPG (*.jpg)")
                    );
        if (!m_fileName.isEmpty()) {
            setLastPath(m_fileName);
            graphicsWidget->setBackgroundBrush(QImage(m_fileName));
            graphicsWidget->setCacheMode(QGraphicsView::CacheBackground);
            statusMessage( tr("BackgroundImage on.") );
        }
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
    tips+=tr("You can add a new edge between two nodes, by middle-clicking (or pressing both mouse buttons simultanesously) on the first and then on the second node.");
    tips+=tr("You can remove a node by right-clicking on it and selecting Remove.");
    tips+=tr("You can change background color (from the menu Edit > Colors).");
    tips+=tr("Nodes can have the colors of your choice. Just right-click on a node and then select > Options > Change Color. You can select every color supported by the X.org palette.");
    tips+=tr("The tabs on the left dock show information about the network (nodes, edges, density, etc) as well as information about any node you clicked on (inDegrees, outDegrees, clustering).");
    tips+=tr("You can move a node easily by dragging it with your mouse.");
    tips+=tr("SocNetV can save the positions of the nodes in a network, if you save it in Pajek/GraphML format.");
    tips+=tr("You can apply layout algorithms on the network from the menu Layout or by clicking on the Dock > Layout tab checkboxes");
    tips+=tr("You can change the label of node by right-clicking on it, and selecting Options > Change Label.");
    tips+=tr("All basic operations of SocNetV are available from the dock on the left, or by right-clicking on a node or an Edge.");
    tips+=tr("Node information is displayed on the Status bar, when you left-click on it.");
    tips+=tr("Edge information is displayed on the Status bar, when you left-click on it.");

    tipsCounter = 16;
}




/**
    Loads the HTML Help file and displays it via system browser
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
        // MacOS: assumes manual dir in socnetv.app/Contents/
        // before deploy copy there the manual dir
        if (d.cd("../manual") ) {         // for Mac
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

    QDesktopServices::openUrl(QUrl::fromLocalFile(helpPath));

}



/**
    Displays the following message!!
*/
void MainWindow::slotHelpAbout(){
    int randomCookie=rand()%fortuneCookiesCounter;//createFortuneCookies();
QString BUILD="Fri Jun  5 17:05:15 EEST 2015";
    QMessageBox::about( this, "About SocNetV",
                        "<b>Soc</b>ial <b>Net</b>work <b>V</b>isualizer (SocNetV)"
                        "<p><b>Version</b>: " + VERSION + "</p>"
                        "<p><b>Build</b>: "  + BUILD + " </p>"

                        "<p>(C) 2005-2015 by Dimitris V. Kalamaras"
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



