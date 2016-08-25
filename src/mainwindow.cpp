/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 2.0
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
#include "settingsdialog.h"




bool printDebug = false;


void myMessageOutput (
        QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    Q_UNUSED(context);
    if ( printDebug )
        switch ( type ) {
        case QtDebugMsg:
            fprintf( stderr, "Debug: %s\n", localMsg.constData() );
            break;

#if QT_VERSION >= 0x050500
        case QtInfoMsg:
            fprintf( stderr, "Info: %s\n", localMsg.constData() );
            break;

#endif

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



/**
 * @brief MainWindow::MainWindow
 * @param m_fileName
 * MainWindow contruction method
 */
MainWindow::MainWindow(const QString & m_fileName) {

    appSettings = initSettings();

    qInstallMessageHandler( myMessageOutput);

    setWindowIcon (QIcon(":/images/socnetv.png"));

    this->setMinimumSize(1024,750); //set MW minimum size, before creating canvas

    initView();         //init our network "canvas"

    /** functions that invoke all other construction parts **/
    initActions();      //register and construct menu Actions

    initMenuBar();      //construct the menu

    initToolBar();      //build the toolbar

    initStatusBar();    //add the status bar

    initToolBox();      //build the toolbox

    initWindowLayout();   //init the application window, set layout etc

    initSignalSlots();  //connect signals and slots between app components

    /*  load and initialise default network parameters  */
    qDebug()<<"   initialise default network parameters";

    initNet();

    // Check if user-provided network file on startup
    qDebug() << "MW::MainWindow() Checking if user provided file on startup...";
    if (!m_fileName.isEmpty())
    {
        fileName=m_fileName;
        fileNameNoPath=fileName.split ("/");
        slotNetworkFilePreview( fileName, 0 );
    }

    graphicsWidget->setFocus();

    statusMessage( tr("Welcome to Social Network Visualizer, Version ")+VERSION);

}



MainWindow::~MainWindow() {
    qDebug() << "MW::~MainWindow() Destruct function running...";
    delete printer;
    delete scene;
    delete graphicsWidget;
    qDebug() << "MW::~MainWindow() Destruct function finished - bye!";
}




/**
  * @brief MainWindow::initSettings()
  * Init default (or user-defined) app settings
  *
  */
QMap<QString,QString> MainWindow::initSettings(){
    qDebug()<< "MW::initSettings";

    printDebug = true; // comment it to stop debug override

    firstTime=true;  // becomes false on user IO

    // Create fortune cookies and tips
    createFortuneCookies();
    slotHelpCreateTips();

    // Call slotNetworkAvailableTextCodecs to setup a list of all supported codecs
    qDebug() << "MW::initSettings - calling slotNetworkAvailableTextCodecs" ;
    slotNetworkAvailableTextCodecs();

    qDebug() << "MW::initSettings - creating PreviewForm object and setting codecs list" ;
    previewForm = new PreviewForm(this);
    previewForm->setCodecList(codecs);

    connect (previewForm, &PreviewForm::loadNetworkFileWithCodec,
             this, &MainWindow::slotNetworkFileLoad );

    qDebug() << "MW::initSettings - creating default settings" ;
    settingsDir = QDir::homePath() +QDir::separator() + "socnetv-data" + QDir::separator() ;
    settingsFilePath = settingsDir + "settings.conf";

    // initially they are the same, but dataDir may be changed by the user
    QString dataDir= settingsDir ;



    maxNodes=5000;		//Max nodes used by createRandomNetwork dialogues
    labelDistance=8;
    numberDistance=5;

    bezier=false;

    // hard-coded initial settings to use only on first app load
    // when there are no user defined values
    appSettings["initNodeSize"]= "7";
    appSettings["initNodeColor"]="red";
    appSettings["initEdgeColor"]="black";
    appSettings["initLabelColor"]="darkblue";
    appSettings["initLabelSize"]="7";
    appSettings["initNumberSize"]="7";
    appSettings["initNumberColor"]="black";
    appSettings["initNodeShape"]="circle";
    appSettings["initBackgroundColor"]="white"; //"gainsboro";
    appSettings["initBackgroundImage"]="";
    appSettings["initNodeNumbersVisibility"] = "true";
    appSettings["initNodeLabelsVisibility"] = "false";
    appSettings["initNodeNumbersInside"] = "false";
    appSettings["considerWeights"]="false";
    appSettings["inverseWeights"]="false";
    appSettings["askedAboutWeights"]="false";
    appSettings["printDebug"] = (printDebug) ? "true" : "false";
    appSettings["showProgressBar"] = "false";
    appSettings["showToolBar"] = "true";
    appSettings["showStatusBar"] = "true";
    appSettings["antialiasing"] = "true";
    appSettings["dataDir"]= dataDir ;
    appSettings["lastUsedDirPath"]= dataDir ;
    appSettings["showRightPanel"] = "true";
    appSettings["showLeftPanel"] = "true";
    appSettings["printLogo"] = "true";

    // Try to load settings configuration file
    // First check if our settings folder exist
    QDir socnetvDir(settingsDir);
    if ( !socnetvDir.exists() ) {
        qDebug() << "MW::initSettings -  dir does not exist - create it";
        socnetvDir.mkdir(settingsDir);
    }
    // Then check if the conf file exists inside the folder
    qDebug () << "MW::initSettings - checking for settings file: "
              << settingsFilePath;

    if (!socnetvDir.exists(settingsFilePath)) {
        saveSettings();
    }
    else {
        qDebug()<< "MW::initSettings - settings file exist - Reading it";
        QFile file(settingsFilePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QMessageBox::critical(this, "File Read Error", tr("Error! \n"
                                  "I cannot read the settings file "
                                   "in \n" + settingsFilePath.toLocal8Bit() +
                                   "\n"
                                  "You can continue using SocNetV with default "
                                  "settings but any changes to them will not "
                                  " be saved for future sessions \n"
                                  "Please, check permissions in your home folder "
                                  " and conduct the developer."
                                  ),
                                  QMessageBox::Ok, 0);
           return appSettings;
        }
        QTextStream in(&file);
        QStringList setting;
        while (!in.atEnd()) {
            QString line = in.readLine();
            if (!line.isEmpty()) {
                setting = line.simplified().split('=');
                //qDebug() << "   setting: " <<  setting[0].simplified() << " = " << setting[1].simplified();
                appSettings.insert (setting[0].simplified() , setting[1].simplified() );
            }
        }

        file.close();
    }

    // restore user setting for debug messages
    printDebug = (appSettings["printDebug"] == "true") ? true:false;

    qDebug() << "MW::initSettings - Final settings";
    QMap<QString, QString>::const_iterator i = appSettings.constBegin();
    while (i != appSettings.constEnd()) {
        qDebug() << "setting: " <<  i.key() << " = " << i.value();
        ++i;
    }

    return appSettings;
}



/**
 * @brief MainWindow::saveSettings
 *  Saves default (or user-defined) app settings
 */
void MainWindow::saveSettings() {
    qDebug () << "MW::saveSettings to "<< settingsFilePath;
    QFile file(settingsFilePath);

    // application settings file does not exist - create it
    // this must be the first time SocNetV runs in this computer
    // or the user might have deleted seetings file.
    if (!file.open(QIODevice::WriteOnly ) ) {
        QMessageBox::critical(this,
                              "File Write Error",
                              tr("Error! \n"
                                 "I cannot write the new settings file "
                                 "in \n" + settingsFilePath.toLocal8Bit() +
                                 "\n"
                                 "You can continue using SocNetV with default "
                                 "settings but any changes to them will not "
                                 " be saved for future sessions \n"
                                 "Please, check permissions in your home folder "
                                 " and conduct the developer."
                                 ),
                              QMessageBox::Ok, 0);
        return;
    }
    qDebug()<< "MW::saveSettings - settings file does not exist - Creating it";
    QTextStream out(&file);
    qDebug()<< "MW::saveSettings - writing settings to settings file first ";
    QMap<QString, QString>::const_iterator i = appSettings.constBegin();
    while (i != appSettings.constEnd()) {
        qDebug() << "   setting: " <<  i.key() << " = " << i.value();
        out << i.key() << " = " << i.value() << endl;
        ++i;
    }
    file.close();

}




/**
 * @brief MainWindow::slotOpenSettingsDialog
 * Open Settings dialog
 */
void MainWindow::slotOpenSettingsDialog() {
    qDebug() << "MW;:slotOpenSettingsDialog()";

    // build dialog

    m_settingsDialog = new SettingsDialog( appSettings, this);

    connect( m_settingsDialog, &SettingsDialog::saveSettings,
                     this, &MainWindow::saveSettings);

    connect( m_settingsDialog, &SettingsDialog::setDebugMsgs,
                         this, &MainWindow::slotOptionsDebugMessages);

    connect( m_settingsDialog, &SettingsDialog::setProgressBars,
             this, &MainWindow::slotOptionsProgressBarVisibility);

    connect( m_settingsDialog, &SettingsDialog::setAntialiasing,
             this, &MainWindow::slotOptionsAntialiasing);

    connect( m_settingsDialog, &SettingsDialog::setPrintLogo,
                 this, &MainWindow::slotOptionsEmbedLogoExporting);

    connect( m_settingsDialog, &SettingsDialog::setBgColor,
                     this, &MainWindow::slotBackgroundColor);

    connect( m_settingsDialog, &SettingsDialog::setBgImage,
                     this, &MainWindow::slotOptionsBackgroundImage);

    connect( m_settingsDialog, &SettingsDialog::setToolBar,
             this, &MainWindow::slotOptionsToolbarVisibility);

    connect( m_settingsDialog, &SettingsDialog::setStatusBar,
             this, &MainWindow::slotOptionsStatusBarVisibility);

    connect( m_settingsDialog, &SettingsDialog::setLeftPanel,
             this, &MainWindow::slotOptionsLeftPanelVisibility);

    connect( m_settingsDialog, &SettingsDialog::setRightPanel,
             this, &MainWindow::slotOptionsRightPanelVisibility);

    connect(m_settingsDialog, SIGNAL(setNodeColor(QColor)),
            this, SLOT(slotEditNodeColorAll(QColor)) );

    connect( m_settingsDialog, &SettingsDialog::setNodeShape,
             this, &MainWindow::slotEditNodeShape);

    connect( m_settingsDialog, &SettingsDialog::setNodeSize,
             this, &MainWindow::slotEditNodeSizeAllNormalized);

    connect( m_settingsDialog, &SettingsDialog::setNodeNumbersVisibility,
             this, &MainWindow::slotOptionsNodeNumbersVisibility);


    // show settings dialog
    m_settingsDialog->exec();

    qDebug ()<< appSettings["initBackgroundImage"] ;

}



/**
 * @brief MainWindow::initActions
 * Initializes ALL QActions of the application
 * Take a breath, the listing below is HUGE.
 */
void MainWindow::initActions(){
    printer = new QPrinter;

    /**
    Network menu actions
    */
    networkNew = new QAction(QIcon(":/images/new.png"), tr("&New"),  this);
    networkNew->setShortcut(Qt::CTRL+Qt::Key_N);
    networkNew->setStatusTip(tr("Creates a new network"));
    networkNew->setToolTip(tr("New network (Ctrl+N)"));
    networkNew->setWhatsThis(tr("New\n\nCreates a new network"));
    connect(networkNew, SIGNAL(triggered()), this, SLOT(slotNetworkNew()));

    networkOpen = new QAction(QIcon(":/images/open.png"), tr("&Open"), this);
    networkOpen->setShortcut(Qt::CTRL+Qt::Key_O);
    networkOpen->setToolTip(tr("Open network (Ctrl+O)"));
    networkOpen->setStatusTip(tr("Open GraphML-formatted file of an existing network"));
    networkOpen->setWhatsThis(tr("Open\n\n"
                              "Opens a file of an existing network in GraphML format"));
    connect(networkOpen, SIGNAL(triggered()), this, SLOT(slotNetworkImportGraphML()));


    networkImportPajek = new QAction( QIcon(":/images/open.png"), tr("&Pajek"), this);
    networkImportPajek->setStatusTip(tr("Import Pajek-formatted file"));
    networkImportPajek->setWhatsThis(tr("Import Pajek \n\n"
                                 "Imports a network from a Pajek-formatted file"));
    connect(networkImportPajek, SIGNAL(triggered()), this, SLOT(slotNetworkImportPajek()));


    networkImportSM = new QAction( QIcon(":/images/open.png"), tr("&Adjacency Matrix"), this);
    networkImportSM->setStatusTip(tr("Import Adjacency matrix"));
    networkImportSM->setWhatsThis(tr("Import Sociomatrix \n\n"
                              "Imports a network from an Adjacency matrix-formatted file"));
    connect(networkImportSM, SIGNAL(triggered()), this, SLOT(slotNetworkImportSM()));

    networkImportDot = new QAction( QIcon(":/images/open.png"), tr("GraphViz (.dot)"), this);
    networkImportDot->setStatusTip(tr("Import dot file"));
    networkImportDot->setWhatsThis(tr("Import GraphViz \n\n "
                               "Imports a network from an GraphViz formatted file"));
    connect(networkImportDot, SIGNAL(triggered()),
            this, SLOT(slotNetworkImportDot()));


    networkImportDL = new QAction( QIcon(":/images/open.png"), tr("UCINET (.dl)..."), this);
    networkImportDL->setStatusTip(tr("ImportDL-formatted file (UCINET)"));
    networkImportDL->setWhatsThis(tr("Import UCINET\n\nImports a network from a DL-formatted file"));
    connect(networkImportDL, SIGNAL(triggered()), this, SLOT(slotNetworkImportDL()));


    networkImportList = new QAction( QIcon(":/images/open.png"), tr("&Edge list"), this);
    networkImportList->setStatusTip(tr("Import an edge list file. "));
    networkImportList->setWhatsThis(tr("Import edge list\n\n"
                                "Import a network from an edgelist file. "
                                " The file can be unvalued or valued (see manual)"
                                ));
    connect(networkImportList, SIGNAL(triggered()),
            this, SLOT(slotNetworkImportEdgeList()));


    networkImportTwoModeSM = new QAction( QIcon(":/images/open.png"), tr("&Two Mode Sociomatrix"), this);
    networkImportTwoModeSM->setStatusTip(tr("Import two-mode sociomatrix (affiliation network) file"));
    networkImportTwoModeSM->setWhatsThis(tr("Import Two-Mode Sociomatrix \n\n "
                                     "Imports a two-mode network from a sociomatrix file. "
                                     "Two-mode networks are described by affiliation "
                                     "network matrices, where A(i,j) codes the "
                                     "events/organizations each actor is affiliated with."));
    connect(networkImportTwoModeSM, SIGNAL(triggered()),
            this, SLOT(slotNetworkImportTwoModeSM()));


    networkSave = new QAction(QIcon(":/images/save.png"), tr("&Save"),  this);
    networkSave->setShortcut(Qt::CTRL+Qt::Key_S);
    networkSave->setToolTip(tr("Save network (Ctrl+S)"));
    networkSave->setStatusTip(tr("Save to the current file"));
    networkSave->setWhatsThis(tr("Save.\n\n"
                              "Saves the actual network to the current file"));
    connect(networkSave, SIGNAL(triggered()), this, SLOT(slotNetworkSave()));

    networkSaveAs = new QAction(QIcon(":/images/save.png"), tr("Save &As..."),  this);
    networkSaveAs->setShortcut(Qt::CTRL+Qt::SHIFT+Qt::Key_S);
    networkSaveAs->setStatusTip(tr("Save under a new filename"
                                "Ctrl+Shift+S"));
    networkSaveAs->setWhatsThis(tr("Save As\n\n"
                                "Saves the actual network under a new filename"));
    connect(networkSaveAs, SIGNAL(triggered()), this, SLOT(slotNetworkSaveAs()));

    networkExportBMP = new QAction(QIcon(":/images/image.png"), tr("&BMP..."), this);
    networkExportBMP->setStatusTip(tr("Export to BMP image"));
    networkExportBMP->setWhatsThis(tr("Export BMP \n\n Exports the network to a BMP image"));
    connect(networkExportBMP, SIGNAL(triggered()), this, SLOT(slotNetworkExportBMP()));

    networkExportPNG = new QAction( QIcon(":/images/image.png"), tr("&PNG..."), this);
    networkExportPNG->setStatusTip(tr("Export to PNG image"));
    networkExportPNG->setWhatsThis(tr("Export PNG \n\n Exports the network to a PNG image"));
    connect(networkExportPNG, SIGNAL(triggered()), this, SLOT(slotNetworkExportPNG()));


    networkExportPDF = new QAction( QIcon(":/images/pdf.png"), tr("&PDF..."), this);
    networkExportPDF->setStatusTip(tr("Export to PDF"));
    networkExportPDF->setWhatsThis(tr("Export PDF\n\n Exports the network to a PDF document"));
    connect(networkExportPDF, SIGNAL(triggered()), this, SLOT(slotNetworkExportPDF()));

    networkExportSM = new QAction( QIcon(":/images/save.png"), tr("&Adjacency Matrix"), this);
    networkExportSM->setStatusTip(tr("Export to adjacency matrix file"));
    networkExportSM->setWhatsThis(tr("Export Sociomatrix \n\n"
                              "Exports the network to an "
                              "adjacency matrix-formatted file"));
    connect(networkExportSM, SIGNAL(triggered()), this, SLOT(slotNetworkExportSM()));

    networkExportPajek = new QAction( QIcon(":/images/save.png"), tr("&Pajek"), this);
    networkExportPajek->setStatusTip(tr("Export to Pajek-formatted file"));
    networkExportPajek->setWhatsThis(tr("Export Pajek \n\n "
                                 "Exports the network to a Pajek-formatted file"));
    connect(networkExportPajek, SIGNAL(triggered()), this, SLOT(slotNetworkExportPajek()));


    networkExportList = new QAction( QIcon(":/images/save.png"), tr("&List"), this);
    networkExportList->setStatusTip(tr("Export to List-formatted file. "));
    networkExportList->setWhatsThis(tr("Export List\n\n"
                                "Exports the network to a List-formatted file"));
    connect(networkExportList, SIGNAL(triggered()), this, SLOT(slotNetworkExportList()));

    networkExportDL = new QAction( QIcon(":/images/save.png"), tr("&DL..."), this);
    networkExportDL->setStatusTip(tr("Export to DL-formatted file"));
    networkExportDL->setWhatsThis(tr("Export DL\n\n"
                                     "Exports the active network to a DL-formatted"));
    connect(networkExportDL, SIGNAL(triggered()), this, SLOT(slotNetworkExportDL()));

    networkExportGW = new QAction( QIcon(":/images/save.png"), tr("&GW..."), this);
    networkExportGW->setStatusTip(tr("Export to GW-formatted file"));
    networkExportGW->setWhatsThis(tr("Export\n\n"
                                     "Exports the active network to a GW formatted file"));
    connect(networkExportGW, SIGNAL(triggered()), this, SLOT(slotNetworkExportGW()));

    networkClose = new QAction( tr("&Close"), this);
    networkClose->setStatusTip(tr("Close the actual network"));
    networkClose->setWhatsThis(tr("Close \n\nCloses the actual network"));
    connect(networkClose, SIGNAL(triggered()), this, SLOT(slotNetworkClose()));

    networkPrint = new QAction(QIcon(":/images/print.png"), tr("&Print"), this);
    networkPrint->setShortcut(Qt::CTRL+Qt::Key_P);
    networkPrint->setStatusTip(tr("Send the network to the printer (Ctrl+P)"));
    networkPrint->setWhatsThis(tr("Printing \n\n"
                                  "This function prints whatever is viewable on "
                                  "the canvas. \nTo print the whole network, "
                                  "you might want to zoom-out."));
    connect(networkPrint, SIGNAL(triggered()), this, SLOT(slotNetworkPrint()));

    networkQuit = new QAction(QIcon(":/images/exit.png"), tr("E&xit"), this);
    networkQuit->setShortcut(Qt::CTRL+Qt::Key_Q);
    networkQuit->setStatusTip(tr("Quits the application"));
    networkQuit->setWhatsThis(tr("Exit\n\nQuits the application"));
    connect(networkQuit, SIGNAL(triggered()), this, SLOT(close()));


    openTextEditorAct = new QAction(QIcon(":/images/texteditor.png"),
                                    tr("Open Text Editor"),this);
    openTextEditorAct ->setShortcut(Qt::SHIFT+Qt::Key_F5);
    openTextEditorAct->setStatusTip(tr("Opens a simple text editor "
                                       "to take notes, copy/paste network data, etc"
                                       "(Shift+F5)"));
    openTextEditorAct->setWhatsThis(tr("Open Text Editor\n\n"
                                       "Opens the SocNetV text editor where you can "
                                       "copy paste network data, of any supported format, "
                                       "and save to a file. Then you can import that file to SocNetV..."));
    connect(openTextEditorAct, SIGNAL(triggered()), this, SLOT(slotNetworkTextEditor()));


    networkViewFileAct = new QAction(QIcon(":/images/networkfile.png"),
                                     tr("View Loaded File"),this);
    networkViewFileAct ->setShortcut(Qt::Key_F5);
    networkViewFileAct->setStatusTip(tr("Displays the loaded network file (F5)"));
    networkViewFileAct->setWhatsThis(tr("View Loaded File\n\n"
                                        "Displays the file of the loaded network"));
    connect(networkViewFileAct, SIGNAL(triggered()), this, SLOT(slotNetworkFileView()));

    networkViewSociomatrixAct = new QAction(QIcon(":/images/sm.png"),
                                     tr("View Adjacency Matrix"),  this);
    networkViewSociomatrixAct ->setShortcut(Qt::Key_F6);
    networkViewSociomatrixAct->setStatusTip(tr("Display the adjacency matrix of the network. "
                                        "(F6)"));
    networkViewSociomatrixAct->setWhatsThis(tr("View Adjacency Matrix\n\n"
                                        "Displays the adjacency matrix of the active network. \n\n"
                                        "The adjacency matrix of a network is a matrix "
                                        "where each element a(i,j) is equal to the weight "
                                        "of the arc from node i to node j. "
                                        "If the nodes are not connected, then a(i,j)=0. "));
    connect(networkViewSociomatrixAct, SIGNAL(triggered()),
            this, SLOT(slotNetworkViewSociomatrix()));

    networkDataSetSelectAct = new QAction(QIcon(":/images/sm.png"),
                                     tr("Create Known Data Sets"),  this);
    networkDataSetSelectAct ->setShortcut(Qt::Key_F7);
    networkDataSetSelectAct->setStatusTip(tr("Recreate a variety of known data sets."));
    networkDataSetSelectAct->setWhatsThis(tr("Known Data Sets\n\n"
                                        "Recreates some of the most widely used "
                                        "data sets in network analysis studies, i.e. "
                                        "Krackhardt's high-tech managers"));
    connect(networkDataSetSelectAct, SIGNAL(triggered()),
            this, SLOT(slotNetworkDataSetSelect()));


    createErdosRenyiRandomNetworkAct = new QAction(QIcon(":/images/erdos.png"),
                                                   tr("Erdős–Rényi"),  this);
    createErdosRenyiRandomNetworkAct -> setShortcut(
                QKeySequence(Qt::CTRL + Qt::Key_R, Qt::CTRL + Qt::Key_E)
                );
    createErdosRenyiRandomNetworkAct->setStatusTip(tr("Creates a random network "
                                                      "according to the Erdős–Rényi model"));
    createErdosRenyiRandomNetworkAct->setWhatsThis(
                tr("Erdős–Rényi \n\n"
                "Creates a random network either of G(n, p) model or G(n,M) model.\n"
                "In the first, edges are created with Bernoulli trials (probability p).\n"
                "In the second, a graph of exactly M edges is created."));
    connect(createErdosRenyiRandomNetworkAct, SIGNAL(triggered()),
            this, SLOT(slotRandomErdosRenyiDialog()));

    createLatticeNetworkAct = new QAction( QIcon(":/images/net1.png"),
                                           tr("Ring Lattice"), this);
    createLatticeNetworkAct -> setShortcut(
                QKeySequence(Qt::CTRL + Qt::Key_R, Qt::CTRL + Qt::Key_L)
                );
    createLatticeNetworkAct->setStatusTip(tr("Create a ring lattice random network"));
    createLatticeNetworkAct->setWhatsThis(
                tr("Ring Lattice \n\n")+
                tr("A ring lattice is a graph with N nodes each connected to d neighbors, d / 2 on each side."));
    connect(createLatticeNetworkAct, SIGNAL(triggered()), this, SLOT(slotRandomRingLattice()));

    createRegularRandomNetworkAct = new QAction(QIcon(":/images/net.png"), tr("d-Regular"), this);
    createRegularRandomNetworkAct -> setShortcut(
                        QKeySequence(Qt::CTRL + Qt::Key_R, Qt::CTRL + Qt::Key_R)
                        );
    createRegularRandomNetworkAct->setStatusTip(tr("Create a random network where every node has the same degree d."));
    createRegularRandomNetworkAct->setWhatsThis(
                tr("d-Regular \n\n") +
                tr("Creates a random network where each node have the same number of neighbours, aka the same degree d "));
    connect(createRegularRandomNetworkAct, SIGNAL(triggered()), this, SLOT(slotRandomRegularNetwork()));

    createGaussianRandomNetworkAct = new QAction(tr("Gaussian"),	this);
    createGaussianRandomNetworkAct -> setShortcut(
                    QKeySequence(Qt::CTRL + Qt::Key_R, Qt::CTRL + Qt::Key_G)
                    );
    createGaussianRandomNetworkAct->setStatusTip(tr("Create a Gaussian distributed random network"));
    createGaussianRandomNetworkAct->setWhatsThis(tr("Gaussian \n\nCreates a random network of Gaussian distribution"));
    connect(createGaussianRandomNetworkAct, SIGNAL(triggered()), this, SLOT(slotRandomGaussian()));

    createSmallWorldRandomNetworkAct = new QAction(QIcon(":/images/sw.png"), tr("Small World"),	this);
    createSmallWorldRandomNetworkAct-> setShortcut(
                QKeySequence(Qt::CTRL + Qt::Key_R, Qt::CTRL + Qt::Key_W)
                );
    createSmallWorldRandomNetworkAct->setStatusTip(tr("Create a random network with small world properties"));
    createSmallWorldRandomNetworkAct ->
            setWhatsThis(
                tr("Small World \n\n") +
                tr("A Small World, according to the Watts and Strogatz model, "
                   "is a random network with short average path lengths and high clustering coefficient."));
    connect(createSmallWorldRandomNetworkAct, SIGNAL(triggered()), this, SLOT(slotRandomSmallWorldDialog()));

    createScaleFreeRandomNetworkAct = new QAction(
                QIcon(":/images/scalefree.png"), tr("Scale-free"),	this);

    createScaleFreeRandomNetworkAct->setShortcut(
                QKeySequence(Qt::CTRL + Qt::Key_R, Qt::CTRL + Qt::Key_S)
                );
    createScaleFreeRandomNetworkAct->setStatusTip(
                tr("Create a random network with power-law degree distribution."));
    createScaleFreeRandomNetworkAct->
            setWhatsThis(
                tr("Scale-free (power-law)\n\n") +
                tr("A scale-free network is a network whose degree distribution follows a power law."
                   " This method generates random scale-free networks according to the "
                   " Barabási–Albert (BA) model using a preferential attachment mechanism."));
    connect(createScaleFreeRandomNetworkAct, SIGNAL(triggered()),
            this, SLOT(slotRandomScaleFreeDialog()));



    webCrawlerAct = new QAction(QIcon(":/images/spider.png"), tr("Web Crawler"),	this);
    webCrawlerAct->setShortcut(Qt::SHIFT+Qt::Key_C);
    webCrawlerAct->setEnabled(true);
    webCrawlerAct->setStatusTip(tr("Create a network from all links found in a given website"
                                   "Shift+C"));
    webCrawlerAct->setWhatsThis(tr("Web Crawler \n\n"
                                   "A Web crawler is a built-in bot, which "
                                   "starts with a given URL (website or webpage) "
                                   "to visit. As the algorithm crawls this webpage, "
                                   "it identifies all the links in the page and adds "
                                   "them to a list of URLs (called frontier). "
                                   "Then, all the URLs from the frontier are "
                                   "recursively visited. You must provide maximum "
                                   "recursion level (how many URLs from the frontier "
                                   "will be visited) and maximum running time, along "
                                   "with the initial web address..."));
    connect(webCrawlerAct, SIGNAL(triggered()), this, SLOT(slotNetworkWebCrawlerDialog()));


    /**
    Edit menu actions
    */

    editRelationNextAct = new QAction(QIcon(":/images/nextrelation.png"),
                                  tr("Next Relation"),  this);
    editRelationNextAct->setShortcut(Qt::ALT + Qt::Key_Right);
    editRelationNextAct->setToolTip(tr("Goto next graph relation (ALT+Right)"));
    editRelationNextAct->setStatusTip(tr("Loads the next relation of the network (if any)."));
    editRelationNextAct->setWhatsThis(tr("Next Relation\n\nLoads the next relation of the network (if any)"));

    editRelationPreviousAct = new QAction(QIcon(":/images/prevrelation.png"),
                                      tr("Previous Relation"),  this);
    editRelationPreviousAct->setShortcut(Qt::ALT + Qt::Key_Left);
    editRelationPreviousAct->setToolTip(
                tr("Goto previous graph relation (ALT+Left)"));
    editRelationPreviousAct->setStatusTip(
                tr("Loads the previous relation of the network (if any)."));
    editRelationPreviousAct->setWhatsThis(
                tr("Previous Relation\n\n"
                   "Loads the previous relation of the network (if any)"));

    editRelationAddAct = new QAction(QIcon(":/images/addrelation.png"),
                                      tr("Add New Relation"),  this);
    editRelationAddAct->setShortcut(Qt::ALT + Qt::CTRL + Qt::Key_N);
    editRelationAddAct->setToolTip(
                tr("Add a new relation to the active graph (Ctrl+Shift+N)"));
    editRelationAddAct->setStatusTip(
                tr("Adds a new relation to the network. "
                   "Nodes will be preserved, edges will be removed. "));
    editRelationAddAct->setWhatsThis(
                tr("Add New Relation\n\n"
                   "Adds a new relation to the active network. "
                   "Nodes will be preserved, edges will be removed. "));


    zoomInAct = new QAction(QIcon(":/images/zoomin.png"), tr("Zoom In"), this);
    zoomInAct->setStatusTip(tr("Zoom in. Better, use the canvas button or press Ctrl++ or press Cltr and use mouse wheel."));
    zoomInAct->setToolTip(tr("Zoom in. Better, use the canvas button or (Ctrl++)"));
    zoomInAct->setWhatsThis(tr("Zoom In.\n\nZooms in the actual network"));
    connect(zoomInAct, SIGNAL(triggered()), graphicsWidget, SLOT( zoomIn()) );

    zoomOutAct = new QAction(QIcon(":/images/zoomout.png"), tr("Zoom Out"), this);
    zoomOutAct->setStatusTip(tr("Zoom out. Better, use the canvas button or press Ctrl+- or press Cltr and use mouse wheel."));
    zoomOutAct->setToolTip(tr("Zoom in. Better, use the canvas button or (Ctrl+-)"));
    zoomOutAct->setWhatsThis(tr("Zoom Out.\n\nZooms out of the actual network"));
    connect(zoomOutAct, SIGNAL(triggered()), graphicsWidget, SLOT( zoomOut()) );

    editRotateLeftAct = new QAction(QIcon(":/images/rotateleft.png"), tr("Rotate counterclockwise"), this);
    editRotateLeftAct->setToolTip(tr("Rotate counterclockwise. Better, use the canvas button or (Ctrl+Left Arrow)"));
    editRotateLeftAct->setStatusTip(tr("Rotate counterclockwise. Better, use the canvas button or Ctrl+Left Arrow"));
    editRotateLeftAct ->setWhatsThis(tr("Rotates the network counterclockwise (Ctrl+Left Arrow)"));
    connect(editRotateLeftAct, SIGNAL(triggered()), graphicsWidget, SLOT( rotateLeft()) );

    editRotateRightAct = new QAction(QIcon(":/images/rotateright.png"), tr("Rotate clockwise"), this);
    editRotateRightAct->setStatusTip(tr("Rotate clockwise. Better, use the canvas button or (Ctrl+Right Arrow)"));
    editRotateRightAct->setToolTip(tr("Rotate clockwise. Better, use the canvas button or (Ctrl+Right Arrow)"));
    editRotateRightAct ->setWhatsThis(tr("Rotates the network clockwise (Ctrl+Right Arrow)"));
    connect(editRotateRightAct, SIGNAL(triggered()), graphicsWidget, SLOT( rotateRight()) );

    editResetSlidersAct = new QAction(QIcon(":/images/reset.png"), tr("Reset Zoom and Rotation"), this);
    editResetSlidersAct->setStatusTip(tr("Reset zoom and rotation to zero (Ctrl+0)"));
    editResetSlidersAct->setToolTip(tr("Reset zoom and rotation to zero (Ctrl+0)"));
    editResetSlidersAct->setWhatsThis(tr("Reset zoom and rotation to zero (Ctrl+0)"));
    connect(editResetSlidersAct, SIGNAL(triggered()), graphicsWidget, SLOT( reset()) );


    editNodeSelectAllAct = new QAction(QIcon(":/images/selectall.png"), tr("Select All"), this);
    editNodeSelectAllAct->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_A));
    editNodeSelectAllAct->setStatusTip(tr("Select all nodes"));
    editNodeSelectAllAct->setWhatsThis(tr("Select All\n\nSelects all nodes in the network"));
    connect(editNodeSelectAllAct, SIGNAL(triggered()), this, SLOT(slotEditNodeSelectAll()));

    editNodeSelectNoneAct = new QAction(QIcon(":/images/selectnone.png"), tr("Deselect all"), this);
    editNodeSelectNoneAct->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_A));
    editNodeSelectNoneAct->setStatusTip(tr("Deselect all nodes"));
    editNodeSelectNoneAct->setWhatsThis(tr("Deselect all\n\n Clears the node selection"));
    connect(editNodeSelectNoneAct, SIGNAL(triggered()), this, SLOT(slotEditNodeSelectNone()));

    editNodeFindAct = new QAction(QIcon(":/images/find.png"), tr("Find Node"), this);
    editNodeFindAct->setShortcut(Qt::CTRL + Qt::Key_F);
    editNodeFindAct->setStatusTip(tr("Find and highlight a node by number or label. "
                                 "Press Ctrl+F again to undo."));
    editNodeFindAct->setWhatsThis(tr("Find Node\n\nFinds a node with a given number or label and doubles its size. Ctrl+F again resizes back the node"));
    connect(editNodeFindAct, SIGNAL(triggered()), this, SLOT(slotEditNodeFind()) );

    editNodeAddAct = new QAction(QIcon(":/images/add.png"), tr("Add Node"), this);
    editNodeAddAct->setShortcut(Qt::CTRL + Qt::Key_Period);
    editNodeAddAct->setStatusTip(tr("Add a new node"));
    editNodeAddAct->setWhatsThis(tr("Add Node\n\n"
                                    "Adds a new node to the active network"));
    connect(editNodeAddAct, SIGNAL(triggered()), this, SLOT(slotEditNodeAdd()));

    editNodeRemoveAct = new QAction(QIcon(":/images/remove.png"),tr("Remove Node"), this);
    editNodeRemoveAct ->setShortcut(Qt::CTRL + Qt::ALT + Qt::Key_Period);
    //Single key shortcuts with backspace or del do no work in Mac http://goo.gl/7hz7Dx
    editNodeRemoveAct->setStatusTip(tr("Remove a node"));
    editNodeRemoveAct->setWhatsThis(tr("Remove Node\n\n"
                                       "Removes an existing node from the network"));
    connect(editNodeRemoveAct, SIGNAL(triggered()), this, SLOT(slotEditNodeRemove()));

    editNodePropertiesAct = new QAction(QIcon(":/images/properties.png"),tr("Node Properties"), this);
    editNodePropertiesAct ->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_Period );
    editNodePropertiesAct->setStatusTip(tr("Open node properties dialog -- "
                                           "There must be some nodes on the canvas!"));
    editNodePropertiesAct->setWhatsThis(tr("Node Properties\n\n"
                                           "Opens node properties dialog to edit label, size, color, shape etc"));
    connect(editNodePropertiesAct, SIGNAL(triggered()), this, SLOT(slotEditNodePropertiesDialog()));


    editNodeColorAll = new QAction(QIcon(":/images/nodecolor.png"), tr("Change all Nodes Colors (this session)"),	this);
    editNodeColorAll->setStatusTip(tr("Choose a new color for all nodes (for this session only)."));
    editNodeColorAll->setWhatsThis(tr("Nodes Color\n\n"
                                      "Changes all nodes color at once. \n"
                                      "This setting will apply to this session only. \n"
                                      "To permanently change it, use Settings & Preferences"));
    connect(editNodeColorAll, SIGNAL(triggered()), this, SLOT(slotEditNodeColorAll()) );

    editNodeSizeAllAct = new QAction(QIcon(":/images/resize.png"), tr("Change all Nodes Size (this session)"),	this);
    editNodeSizeAllAct->setStatusTip(tr("Change the size of all nodes (for this session only)"));
    editNodeSizeAllAct->setWhatsThis(tr("Nodes Size\n\n"
                                        "Click to select and apply a new size for all nodes at once. \n"
                                        "This setting will apply to this session only. \n"
                                        "To permanently change it, use Settings & Preferences"));
    connect(editNodeSizeAllAct, SIGNAL(triggered()), this, SLOT(slotEditNodeSizeAll()) );

    editNodeShapeAll = new QAction( tr("Change all Nodes Shape (this session)"),	this);
    editNodeShapeAll->setStatusTip(tr("Change the shape of all nodes (for this session only)"));
    editNodeShapeAll->setWhatsThis(tr("Nodes Shape\n\n"
                                      "Click to select and apply a new shape for all nodes at once."
                                      "This setting will apply to this session only. \n"
                                      "To permanently change it, use Settings & Preferences"));
    connect(editNodeShapeAll, SIGNAL(triggered()), this, SLOT(slotEditNodeShapeAll()) );


    editNodeNumbersSizeAct = new QAction( tr("Change all Numbers Size (this session)"),	this);
    editNodeNumbersSizeAct->setStatusTip(tr("Change the font size of the numbers of all nodes"
                                            "(for this session only)"));
    editNodeNumbersSizeAct->setWhatsThis(tr("Numbers Size\n\n"
                                            "Click to select and apply a new font size for all node numbers"
                                            "This setting will apply to this session only. \n"
                                            "To permanently change it, use Settings & Preferences"));
    connect(editNodeNumbersSizeAct, SIGNAL(triggered()), this, SLOT(slotEditNodeNumbersSize()) );


    editNodeNumbersColorAct = new QAction( tr("Change all Numbers Color (this session)"),	this);
    editNodeNumbersColorAct->setStatusTip(tr("Change the color of the numbers of all nodes."
                                              "(for this session only)"));
    editNodeNumbersColorAct->setWhatsThis(tr("Numbers Color\n\n"
                                              "Click to select and apply a new color "
                                              "to all node numbers."
                                              "This setting will apply to this session only. \n"
                                              "To permanently change it, use Settings & Preferences"));
    connect(editNodeNumbersColorAct, SIGNAL(triggered()), this, SLOT(slotEditNodeNumbersColor()));

    editNodeLabelsSizeAct = new QAction( tr("Change all Labels Size (this session)"), this);
    editNodeLabelsSizeAct->setStatusTip(tr("Change the font size of the labels of all nodes"
                                           "(for this session only)"));
    editNodeLabelsSizeAct->setWhatsThis(tr("Labels Size\n\n"
                                           "Click to select and apply a new font-size to all node labels"
                                           "This setting will apply to this session only. \n"
                                           "To permanently change it, use Settings & Preferences"));
    connect(editNodeLabelsSizeAct, SIGNAL(triggered()), this, SLOT(slotEditNodeLabelsSize()) );

    changeAllLabelsColorAct = new QAction( tr("Change all Labels Colors (this session)"),	this);
    changeAllLabelsColorAct->setStatusTip(tr("Change the color of the labels of all nodes "
                                             "(for this session only)"));
    changeAllLabelsColorAct->setWhatsThis(tr("Labels Color\n\n"
                                             "Click to select and apply a new color to all node labels."
                                             "This setting will apply to this session only. \n"
                                             "To permanently change it, use Settings & Preferences"));
    connect(changeAllLabelsColorAct, SIGNAL(triggered()), this, SLOT(slotEditNodeLabelsColor()));

    editEdgeAddAct = new QAction(QIcon(":/images/plines.png"), tr("Add Edge"),this);
    editEdgeAddAct->setShortcut(Qt::CTRL + Qt::Key_Slash);
    editEdgeAddAct->setStatusTip(tr("Adds a directed edge from a node to another"));
    editEdgeAddAct->setWhatsThis(tr("Add Edge\n\nAdds a directed edge from a node to another"));
    connect(editEdgeAddAct, SIGNAL(triggered()), this, SLOT(slotEditEdgeAdd()));

    editEdgeRemoveAct = new QAction(QIcon(":/images/disconnect.png"), tr("Remove"), this);
    editEdgeRemoveAct ->setShortcut(Qt::CTRL + Qt::ALT + Qt::Key_Slash);
    editEdgeRemoveAct->setStatusTip(tr("Removes an Edge"));
    editEdgeRemoveAct->setWhatsThis(tr("Remove Edge\n\nRemoves an Edge from the network"));
    connect(editEdgeRemoveAct, SIGNAL(triggered()), this, SLOT(slotEditEdgeRemove()));

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
    //filterNodesAct->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_X, Qt::CTRL + Qt::Key_F));
    filterNodesAct->setStatusTip(tr("Filters Nodes of some value out of the network"));
    filterNodesAct->setWhatsThis(tr("Filter Nodes\n\nFilters Nodes of some value out of the network."));
    connect(filterNodesAct, SIGNAL(triggered()), this, SLOT(slotFilterNodes()));

    filterIsolateNodesAct = new QAction(tr("Filter Isolate Nodes"), this);
    filterIsolateNodesAct -> setEnabled(true);
    filterIsolateNodesAct -> setCheckable(true);
    filterIsolateNodesAct -> setChecked(false);
    filterIsolateNodesAct -> setShortcut(QKeySequence(Qt::CTRL + Qt::Key_X, Qt::CTRL + Qt::Key_F));
    filterIsolateNodesAct -> setStatusTip(tr("Filters nodes with no edges"));
    filterIsolateNodesAct -> setWhatsThis(tr("Filter Isolate Nodes\n\n Enables or disables displaying of isolate nodes. Isolate nodes are those with no edges..."));
    connect(filterIsolateNodesAct, SIGNAL(toggled(bool)), this, SLOT(slotFilterIsolateNodes(bool)));

    filterEdgesAct = new QAction(tr("Filter Edges by weight"), this);
    filterEdgesAct -> setEnabled(true);
    filterEdgesAct -> setShortcut(QKeySequence(Qt::CTRL + Qt::Key_E, Qt::CTRL + Qt::Key_F));
    filterEdgesAct -> setStatusTip(tr("Filters Edges of some weight out of the network"));
    filterEdgesAct -> setWhatsThis(tr("Filter Edges\n\nFilters Edge of some specific weight out of the network."));
    connect(filterEdgesAct , SIGNAL(triggered()), this, SLOT(slotShowFilterEdgesDialog()));

    changeBackColorAct = new QAction(QIcon(":/images/color.png"), tr("Change Background Color"), this);
    changeBackColorAct->setStatusTip(tr("Click to change the background color"));
    changeBackColorAct->setWhatsThis(tr("Background\n\nChanges background color"));
    connect(changeBackColorAct, SIGNAL(triggered()), this, SLOT(slotBackgroundColor()));


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
    randLayoutAct -> setShortcut(Qt::CTRL+Qt::SHIFT+Qt::Key_0);
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
    springLayoutAct->setStatusTip(tr("All nodes repel each other while the connected ones are attracted as if connected by springs."));
    springLayoutAct->setWhatsThis(tr("Spring Embedder Layout\n\n In this model, nodes are regarded as physical bodies (i.e. electrons) which exert repelling forces to each other, while edges are springs connecting adjacents nodes. Non-adjacent nodes repel each other while connected nodes are The algorithm continues until the system retains an equilibrium state in which all forces cancel each other. "));
    connect(springLayoutAct, SIGNAL(triggered(bool)), this, SLOT(slotLayoutSpringEmbedder()));

    FRLayoutAct= new QAction( tr("Fruchterman-Reingold"),	this);
    FRLayoutAct->setShortcut(tr("Alt+2"));
    FRLayoutAct->setStatusTip(tr("Repelling forces between all nodes, and attracting forces between adjacent nodes."));
    FRLayoutAct->setWhatsThis(tr("Fruchterman-Reingold Layout\n\n Embeds a layout all nodes according to a model in which	repelling forces are used between every pair of nodes, while attracting forces are used only between adjacent nodes. The algorithm continues until the system retains its equilibrium state where all forces cancel each other."));
    connect(FRLayoutAct, SIGNAL(triggered()), this, SLOT(slotLayoutFruchterman()));





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
    optionsNodeNumbersVisibilityAct = new QAction( tr("Display Node Numbers"), this );
    optionsNodeNumbersVisibilityAct->setStatusTip(
                tr("Toggle displaying of node numbers (this session only)"));
    optionsNodeNumbersVisibilityAct->setWhatsThis(
                tr("Display Node Numbers\n\n"
                   "Enables or disables displaying of node numbers\n"
                   "This setting will apply to this session only. \n"
                   "To permanently change it, use Settings & Preferences"));
    optionsNodeNumbersVisibilityAct->setCheckable (true);
    optionsNodeNumbersVisibilityAct->setChecked (
                ( appSettings["initNodeNumbersVisibility"] == "true" ) ? true: false );
    connect(optionsNodeNumbersVisibilityAct, SIGNAL(toggled(bool)),
            this, SLOT(slotOptionsNodeNumbersVisibility(bool)));


    optionsNodeNumbersInsideAct = new QAction(tr("Display Numbers Inside Nodes"),	this );
    optionsNodeNumbersInsideAct->setStatusTip(
                tr("Toggle displaying of numbers inside nodes (this session only)"));
    optionsNodeNumbersInsideAct->setWhatsThis(
                tr("Display Numbers Inside Nodes\n\n"
                   "Enables or disables displaying node numbers inside nodes.\n"
                   "This setting will apply to this session only. \n"
                   "To permanently change it, use Settings & Preferences"));
    optionsNodeNumbersInsideAct->setCheckable (true);
    optionsNodeNumbersInsideAct->setChecked(
                ( appSettings["initNodeNumbersInside"] == "true" ) ? true: false );
    connect(optionsNodeNumbersInsideAct, SIGNAL(toggled(bool)),
            this, SLOT(slotOptionsNodeNumbersInside(bool)));


    optionsNodeLabelsVisibilityAct= new QAction(tr("Display Node Labels"),	this );
    optionsNodeLabelsVisibilityAct->setStatusTip(
                tr("Toggle displaying of node labels (this session only"));
    optionsNodeLabelsVisibilityAct->setWhatsThis(
                tr("Display Node Labels\n\n"
                   "Enables or disables node labels.\n"
                   "This setting will apply to this session only. \n"
                   "To permanently change it, use Settings & Preferences"));
    optionsNodeLabelsVisibilityAct->setCheckable (true);
    optionsNodeLabelsVisibilityAct->setChecked(
                ( appSettings["initNodeLabelsVisibility"] == "true" ) ? true: false );
    connect(optionsNodeLabelsVisibilityAct, SIGNAL(toggled(bool)),
            this, SLOT(slotOptionsNodeLabelsVisibility(bool)));


    displayEdgesAct = new QAction(tr("Display Edges"), this);
    displayEdgesAct->setStatusTip(tr("Toggle displaying edges"));
    displayEdgesAct->setWhatsThis(
                tr("Display Edges\n\n"
                "Enables or disables displaying of edges"
                "This setting will apply to this session only. \n"
                "To permanently change it, use Settings & Preferences"));
    displayEdgesAct->setCheckable(true);
    displayEdgesAct->setChecked(true);
    connect(displayEdgesAct, SIGNAL(toggled(bool)),
            this, SLOT(slotOptionsEdgesVisibility(bool)) );


    displayEdgesWeightNumbersAct = new QAction(tr("Display edge Weights"),	this);
    displayEdgesWeightNumbersAct->setStatusTip(
                tr("Toggles displaying of numbers of Edges weights (this session only)"));
    displayEdgesWeightNumbersAct->setWhatsThis(
                tr("Display edge Weights\n\n"
                   "Enables or disables displaying edge weight numbers.\n"
                   "This setting will apply to this session only. \n"
                   "To permanently change it, use Settings & Preferences"));
    displayEdgesWeightNumbersAct->setCheckable(true);
    displayEdgesWeightNumbersAct->setChecked(false);
    connect(displayEdgesWeightNumbersAct, SIGNAL(toggled(bool)),
            this, SLOT(slotOptionsEdgeWeightNumbersVisibility(bool)) );

    considerEdgeWeightsAct = new QAction(tr("Consider edge Weights in calculations"),	this);
    considerEdgeWeightsAct->
            setStatusTip(
                tr("Toggle considering edge Weights during calculations "
                   "(i.e. distances, centrality, etc)"));
    considerEdgeWeightsAct->
            setWhatsThis(
                tr("Consider edge weights in calculations\n\n"
                   "Enables or disables considering edge weights during "
                   "calculations (i.e. distances, centrality, etc).\n"
                   "This setting will apply to this session only. \n"
                   "To permanently change it, use Settings & Preferences"));
    considerEdgeWeightsAct->setCheckable(true);
    considerEdgeWeightsAct->setChecked(false);
    connect(considerEdgeWeightsAct, SIGNAL(triggered(bool)),
            this, SLOT(slotOptionsEdgeWeightsDuringComputation(bool)) );

    displayEdgesArrowsAct = new QAction( tr("Display edge Arrows"),this);
    displayEdgesArrowsAct->setStatusTip(
                tr("Toggles displaying directional Arrows on edges"));
    displayEdgesArrowsAct->setWhatsThis(
                tr("Display edge Arrows\n\n"
                   "Enables or disables displaying of arrows on edges.\n "
                   "Useful if all links are reciprocal (undirected graph).\n"
                   "This setting will apply to this session only. \n"
                   "To permanently change it, use Settings & Preferences"));
    displayEdgesArrowsAct->setCheckable(true);
    displayEdgesArrowsAct->setChecked(true);
    connect(displayEdgesArrowsAct, SIGNAL(toggled(bool)),
            this, SLOT(slotOptionsEdgeArrrowsVisibility(bool)) );

    drawEdgesWeightsAct = new QAction( tr("Edge thickness reflects weight"), this);
    drawEdgesWeightsAct->setStatusTip(tr("Draw edges as thick as their weights (if specified)"));
    drawEdgesWeightsAct->setWhatsThis(
                tr("Edge thickness reflects weight\n\n"
                   "Click to toggle having all edges as thick as their weight (if specified)"));
    drawEdgesWeightsAct->setCheckable(true);
    drawEdgesWeightsAct->setChecked(false);
    drawEdgesWeightsAct->setEnabled(false);
    connect(drawEdgesWeightsAct, SIGNAL(toggled(bool)),
            this, SLOT(slotOptionsEdgesThicknessPerWeight()) );

    drawEdgesBezier = new QAction( tr("Bezier Curves"),	this);
    drawEdgesBezier->setStatusTip(tr("Draw Edges as Bezier curves"));
    drawEdgesBezier->setWhatsThis(
                tr("Edges Bezier\n\n"
                   "Enable or disables drawing Edges as Bezier curves."
                   "This setting will apply to this session only. \n"
                   "To permanently change it, use Settings & Preferences"));
    drawEdgesBezier->setCheckable(true);
    drawEdgesBezier->setChecked (false);
    drawEdgesBezier->setEnabled(false);
    connect(drawEdgesBezier, SIGNAL(toggled(bool)), this, SLOT(slotOptionsEdgesBezier(bool)) );


    backgroundImageAct = new QAction(tr("Background Image (this session)"),	this);
    backgroundImageAct->setStatusTip(
                tr("Select and display a custom image in the background"
                   "(for this session only)"));
    backgroundImageAct->setWhatsThis(
                tr("Background image\n\n "
                   "Enable to select an image file from your computer, "
                   "which will be displayed in the background instead of plain color."
                   "This setting will apply to this session only. \n"
                   "To permanently change it, use Settings & Preferences"));
    backgroundImageAct->setCheckable(true);
    backgroundImageAct->setChecked(false);
    connect(backgroundImageAct, SIGNAL(toggled(bool)), this, SLOT(slotOptionsBackgroundImageSelect(bool)));

    openSettingsAct = new QAction(QIcon(":/images/appsettings.png"), tr("Settings"),	this);
    openSettingsAct->setShortcut(Qt::CTRL + Qt::Key_Comma);
    openSettingsAct->setEnabled(true);
    openSettingsAct->setStatusTip(
                tr("Open Settings dialog where you can save your preferences "
                   "for all future sessions"));
    openSettingsAct->setWhatsThis(
                tr("Settings\n\n "
                   "Opens the Settings dialog where you can edit and save settings "
                   "permanently for all subsequent sessions."));
    connect(openSettingsAct, SIGNAL(triggered()),
            this, SLOT(slotOpenSettingsDialog()));




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
    connect(tipsApp, SIGNAL(triggered()), this, SLOT(slotHelpTips()));

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
    networkMenu -> addAction(networkNew);
    networkMenu -> addAction(networkOpen);
    importSubMenu = new QMenu(tr("Import ..."));
    importSubMenu -> setIcon(QIcon(":/images/import.png"));
    importSubMenu -> addAction(networkImportPajek);
    importSubMenu -> addAction(networkImportSM);
    importSubMenu -> addAction(networkImportTwoModeSM);
    importSubMenu -> addAction(networkImportList);
    importSubMenu -> addAction(networkImportDL);
    importSubMenu -> addAction(networkImportDot);
    networkMenu ->addMenu (importSubMenu);

    networkMenu -> addSeparator();
    networkMenu -> addAction (openTextEditorAct);
    networkMenu -> addAction (networkViewFileAct);
    networkMenu -> addSeparator();
    networkMenu -> addAction (networkViewSociomatrixAct);
    networkMenu -> addSeparator();

    networkMenu -> addAction (networkDataSetSelectAct);
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
    networkMenu  -> addAction(networkSave);
    networkMenu  -> addAction(networkSaveAs);
    networkMenu  -> addSeparator();

    exportSubMenu = networkMenu  -> addMenu(tr("Export..."));

    exportSubMenu -> addAction (networkExportBMP);
    exportSubMenu -> addAction (networkExportPNG);
    exportSubMenu -> addAction (networkExportPDF);
    exportSubMenu -> addSeparator();
    exportSubMenu -> addAction (networkExportSM);
    exportSubMenu -> addAction (networkExportPajek);
    //exportSubMenu -> addAction (networkExportList);
    //exportSubMenu -> addAction (networkExportDL);
    //exportSubMenu -> addAction (networkExportGW);

    networkMenu  -> addSeparator();
    networkMenu  -> addAction(networkPrint);
    networkMenu  -> addSeparator();
    networkMenu  -> addAction(networkClose);
    networkMenu  -> addAction(networkQuit);




    /** menuBar entry editMenu */

    editMenu = menuBar()->addMenu(tr("&Edit"));

    editMenu -> addAction (editRelationPreviousAct);
    editMenu -> addAction (editRelationNextAct);
    editMenu -> addAction (editRelationAddAct);

    editMenu -> addSeparator();

    editMenu -> addAction ( zoomInAct );
    editMenu -> addAction ( zoomOutAct );

    editMenu -> addSeparator();

    editMenu -> addAction ( editRotateLeftAct );
    editMenu -> addAction ( editRotateRightAct );

    editMenu -> addSeparator();
    editMenu -> addAction (editResetSlidersAct );

    editMenu -> addSeparator();
    editNodeMenu = new QMenu(tr("Node..."));
    editNodeMenu -> setIcon(QIcon(":/images/node.png"));
    editMenu -> addMenu ( editNodeMenu );
    editNodeMenu -> addAction (editNodeSelectAllAct);
    editNodeMenu -> addAction (editNodeSelectNoneAct);

    editNodeMenu -> addSeparator();

    editNodeMenu -> addAction (editNodeFindAct);
    editNodeMenu -> addAction (editNodeAddAct);
    editNodeMenu -> addAction (editNodeRemoveAct);

    editNodeMenu -> addSeparator();

    editNodeMenu -> addAction (editNodePropertiesAct);

    editNodeMenu -> addSeparator();

    editNodeMenu -> addAction (editNodeColorAll);
    editNodeMenu -> addAction (editNodeSizeAllAct);
    editNodeMenu -> addAction (editNodeShapeAll);
    editNodeMenu -> addAction (editNodeNumbersSizeAct);
    editNodeMenu -> addAction (editNodeLabelsSizeAct);


    editEdgeMenu = new QMenu(tr("Edge..."));
    editEdgeMenu -> setIcon(QIcon(":/images/line.png"));
    editMenu-> addMenu (editEdgeMenu);
    editEdgeMenu -> addAction(editEdgeAddAct);
    editEdgeMenu -> addAction(editEdgeRemoveAct);
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
    colorOptionsMenu -> addAction (editNodeColorAll);
    colorOptionsMenu -> addAction (changeAllEdgesColorAct);
    colorOptionsMenu -> addAction (editNodeNumbersColorAct);
    colorOptionsMenu -> addAction (changeAllLabelsColorAct);




    /** menuBar entry: analyze menu */
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



    /** menuBar entry optionsMenu  */
    optionsMenu = menuBar()->addMenu(tr("&Options"));
    nodeOptionsMenu=new QMenu(tr("Nodes..."));
    nodeOptionsMenu -> setIcon(QIcon(":/images/nodes.png"));

    optionsMenu -> addMenu (nodeOptionsMenu);
    nodeOptionsMenu -> addAction (optionsNodeNumbersVisibilityAct);
    nodeOptionsMenu -> addAction (optionsNodeLabelsVisibilityAct);
    nodeOptionsMenu -> addAction (optionsNodeNumbersInsideAct);

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



    optionsMenu -> addSeparator();
    optionsMenu -> addAction (openSettingsAct);



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

    toolBar -> addAction (networkNew);
    toolBar -> addAction (networkOpen);
    toolBar -> addAction (networkSave);
    toolBar -> addAction (networkPrint);
    toolBar -> addSeparator();

    QLabel *labelRotateSpinBox= new QLabel;
    labelRotateSpinBox ->setText(tr("Rotation:"));

    toolBar -> addSeparator();

    //Create relation select widget
    QLabel *labelRelationSelect= new QLabel;
    labelRelationSelect ->setText(tr("Relation:"));
    toolBar -> addWidget (labelRelationSelect);
    toolBar -> addAction (editRelationPreviousAct);
    editRelationChangeCombo = new QComboBox;
    editRelationChangeCombo->setMinimumWidth(180);
    editRelationChangeCombo->setCurrentIndex(0);
    editRelationChangeCombo->setToolTip(
                tr("Displays current relation - Click to change graph relation"));
    editRelationChangeCombo->setStatusTip(
                tr("Displays current relation - Click to change graph relation"));
    editRelationChangeCombo->setWhatsThis(
                tr("Previous Relation\n\n"
                   "Displays current relation - Click to change graph relation (if any)"));

    toolBar -> addWidget(editRelationChangeCombo);
    toolBar -> addAction (editRelationNextAct);
    toolBar -> addAction (editRelationAddAct);

    toolBar -> addSeparator();
    toolBar -> addAction ( QWhatsThis::createAction (this));
    toolBar -> setIconSize(QSize(16,16));
}








/**
 * @brief MainWindow::initToolBox
 * Creates a dock widget for instant menu access
 */
void MainWindow::initToolBox(){

    /*
     *  create widgets for the Controls Tab
     */

    // create 4 buttons for the Edit groupbox
    editNodeAddBt= new QPushButton(QIcon(":/images/add.png"),tr("&Add Node"));
    editNodeAddBt->setFocusPolicy(Qt::NoFocus);
    editNodeAddBt->setMinimumWidth(100);
    editNodeAddBt->setStatusTip( tr("Add a new node to the network.") ) ;
    editNodeAddBt->setToolTip(
                tr("Add a new node to the network (Ctrl+.). \n\n "
                   "You can also create a new node \n"
                   "in a specific position by double-clicking \n")
                );
    editNodeAddBt->setWhatsThis(
                tr("Add new node\n\n"
                   "Adds a new node to the network (Ctrl+.). \n\n "
                   "Alternately, you can create a new node \n"
                   "in a specific position by double-clicking \n"
                   "on that spot of the canvas.")
                );

    removeNodeBt= new QPushButton(QIcon(":/images/remove.png"),tr("&Remove Node"));
    removeNodeBt->setFocusPolicy(Qt::NoFocus);
    removeNodeBt->setMinimumWidth(100);
    removeNodeBt->setStatusTip( tr("Remove a node from the network. ") );
    removeNodeBt->setToolTip(
                tr("Remove a node from the network (Ctrl+Alt+.). ")
                );

    removeNodeBt->setWhatsThis(
                tr("Remove node\n\n"
                   "Removes a node from the network (Ctrl+Alt+.). \n\n "
                   "Alternately, you can remove a node \n"
                   "by right-clicking on it.")
                );

    editEdgeAddBt= new QPushButton(QIcon(":/images/connect.png"),tr("Add &Edge"));
    editEdgeAddBt->setFocusPolicy(Qt::NoFocus);
    editEdgeAddBt->setMinimumWidth(100);
    editEdgeAddBt->setStatusTip(
                tr("Add a new Edge from a node to another. ")
                );
    editEdgeAddBt->setToolTip(
                tr("Add a new Edge from a node to another (Ctrl+/).\n\n "
                   "You can also create an edge between two nodes\n"
                   "by double-clicking or middle-clicking on them consecutively.")
                );
    editEdgeAddBt->setWhatsThis(
                tr("Add edge\n\n"
                   "Adds a new Edge from a node to another (Ctrl+/).\n\n "
                   "Alternately, you can create a new edge between two nodes\n"
                   "by double-clicking or middle-clicking on them consecutively.")
                );

    editEdgeRemoveBt= new QPushButton(QIcon(":/images/disconnect.png"),tr("Remove Edge"));
    editEdgeRemoveBt->setFocusPolicy(Qt::NoFocus);
    editEdgeRemoveBt->setMinimumWidth(100);
    editEdgeRemoveBt->setStatusTip( tr("Remove an Edge from the network ")  );
    editEdgeRemoveBt->setToolTip(
                tr("Remove an Edge from the network (Ctrl+Alt+/)"
                   )
                );
    editEdgeRemoveBt->setWhatsThis(
                tr("Remove edge\n\n"
                   "Removes an Edge from the network  (Ctrl+Alt+/)."
                   "Alternately, you can remove an Edge \n"
                   "by right-clicking on it."
                   )
                );


    //create a grid layout for these buttons
    QGridLayout *buttonsGrid = new QGridLayout;
    buttonsGrid -> addWidget(editNodeAddBt, 0,0);
    buttonsGrid -> addWidget(removeNodeBt, 0,1);
    buttonsGrid -> addWidget(editEdgeAddBt,1,0);
    buttonsGrid -> addWidget(editEdgeRemoveBt,1,1);
    buttonsGrid -> setSpacing(5);
    buttonsGrid -> setContentsMargins(5, 5, 5, 5);

    //create a groupbox "Edit" - Inside, display the grid layout of widgets
    QGroupBox *editGroupBox= new QGroupBox(tr("Edit"));
    editGroupBox->setLayout(buttonsGrid);
    editGroupBox->setMaximumWidth(280);
    editGroupBox->setMinimumHeight(100);


    //create widgets for the "Analysis" box
    QLabel *toolBoxAnalysisGeodesicsSelectLabel = new QLabel;
    toolBoxAnalysisGeodesicsSelectLabel->setText(tr("Distances:"));
    toolBoxAnalysisGeodesicsSelectLabel->setMinimumWidth(115);
    toolBoxAnalysisGeodesicsSelect = new QComboBox;
    toolBoxAnalysisGeodesicsSelect -> setStatusTip(
                tr("Basic graph-theoretic metrics i.e. diameter."));
    toolBoxAnalysisGeodesicsSelect -> setToolTip(
                tr("Compute basic graph-theoretic features of the network, "
                   "i.e. diameter."));
    toolBoxAnalysisGeodesicsSelect -> setWhatsThis(
                tr("Analyze Distances\n\n"
                   "Compute basic graph-theoretic features of the network "
                   "i.e. diameter, eccentricity, distances etc."));
    QStringList geodesicsCommandsList;
    geodesicsCommandsList << "None selected"
                          << "Distance" << "Average Distance"
                          << "Distances Matrix" << "Geodesics Matrix"
                          << "Eccentricity" << "Diameter";
    toolBoxAnalysisGeodesicsSelect->addItems(geodesicsCommandsList);
    toolBoxAnalysisGeodesicsSelect->setMinimumWidth(115);


    QLabel *toolBoxAnalysisConnectivitySelectLabel  = new QLabel;
    toolBoxAnalysisConnectivitySelectLabel->setText(tr("Connectivity:"));
    toolBoxAnalysisConnectivitySelectLabel->setMinimumWidth(115);
    toolBoxAnalysisConnectivitySelect = new QComboBox;
    toolBoxAnalysisConnectivitySelect->setStatusTip(
                tr("'Connectivity' metrics i.e. connectedness, walks, etc."));
    toolBoxAnalysisConnectivitySelect->setToolTip(
                tr("Compute 'connectivity' metrics such as network connectedness, "
                   "walks, reachability etc."));
    toolBoxAnalysisConnectivitySelect->setWhatsThis(
                tr("Analyze Connectivity\\n\n"
                   "Compute 'connectivity' metrics such as network connectedness, "
                   "walks, reachability etc."));
    QStringList connectivityCommands;
    connectivityCommands << "None selected"
                         << "Connectedness" << "Walks of given length"
                         << "Total Walks" << "Reachability Matrix";
    toolBoxAnalysisConnectivitySelect->addItems(connectivityCommands);
    toolBoxAnalysisConnectivitySelect->setMinimumWidth(115);


    QLabel *toolBoxAnalysisClusterabilitySelectLabel  = new QLabel;
    toolBoxAnalysisClusterabilitySelectLabel->setText(tr("Clusterability:"));
    toolBoxAnalysisClusterabilitySelectLabel->setMinimumWidth(115);
    toolBoxAnalysisClusterabilitySelect = new QComboBox;
    toolBoxAnalysisClusterabilitySelect->setStatusTip(
                tr("'Clusterability' metrics, i.e. cliques"));
    toolBoxAnalysisClusterabilitySelect->setToolTip(
                tr("Compute 'clusterability' metrics, such as cliques"));
            toolBoxAnalysisClusterabilitySelect->setWhatsThis(
                        tr("Analyze Clusterability\n\n"
                           "Compute 'clusterability' metrics, such as cliques"));
    QStringList clusterabilityCommands;
    clusterabilityCommands << "None selected"
                         << "Cliques"
                         << "Clustering Coefficient"
                         << "Triad Census";
    toolBoxAnalysisClusterabilitySelect->addItems(clusterabilityCommands);
    toolBoxAnalysisClusterabilitySelect->setMinimumWidth(115);


    QLabel *toolBoxAnalysisProminenceSelectLabel  = new QLabel;
    toolBoxAnalysisProminenceSelectLabel->setText(tr("Prominence:"));
    toolBoxAnalysisProminenceSelectLabel->setMinimumWidth(115);
    toolBoxAnalysisProminenceSelect = new QComboBox;
    toolBoxAnalysisProminenceSelect -> setStatusTip(
                tr("Metrics of how 'prominent' or important each node is.")
                );
    toolBoxAnalysisProminenceSelect -> setToolTip(
                tr("Compute metrics to see how 'prominent' or "
                   "important each actor (node) is inside the network.")
                );
    toolBoxAnalysisProminenceSelect -> setWhatsThis(
                tr("Analyze Prominence\n\n"
                   "Computes various metrics to see how 'prominent' or "
                   "important each actor (node) is inside the network.")
                );
    toolBoxAnalysisProminenceSelect -> setWhatsThis(
                tr("Analyze Prominence\n\n"
                   "Computes various metrics to see how 'prominent' or "
                   "important each actor (node) is inside the network.\n\n"
                "Centrality metrics quantify how central is each node by examining "
                   "its ties and its geodesic distances (shortest path lengths) to other nodes. "
                "Most Centrality indices were designed for undirected graphs.\n\n"
                "Prestige indices focus on \"choices received\" to a node. \n"
                "These indices measure the nominations or ties to each node from all others (or inLinks). "
                "Prestige indices are suitable (and can be calculated only) on directed graphs.")
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
    toolBoxAnalysisProminenceSelect->setMinimumWidth(115);

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
    analysisGrid -> setSpacing(5);
    analysisGrid -> setContentsMargins(15, 5, 15, 5);


    //create a box and set the above layout inside
    QGroupBox *analysisBox= new QGroupBox(tr("Analyze"));
    analysisBox->setMinimumHeight(170);
    analysisBox->setMaximumWidth(280);
    analysisBox->setLayout (analysisGrid );


    //create widgets for the "Visualization By Index" box
    QLabel *toolBoxLayoutByIndexSelectLabel = new QLabel;
    toolBoxLayoutByIndexSelectLabel->setText(tr("Index:"));
    toolBoxLayoutByIndexSelectLabel->setMinimumWidth(110);
    toolBoxLayoutByIndexSelect = new QComboBox;
    toolBoxLayoutByIndexSelect->setStatusTip(tr("Select a prominence-based layout model"));
    toolBoxLayoutByIndexSelect->setToolTip(tr("Apply a prominence-based layout model"));
    toolBoxLayoutByIndexSelect->setWhatsThis(
                tr("Visualize by prominence index\n\n"
                   "Apply a prominence-based layout model to the network "
                   "For instance, you can apply a degree centrality layout. "
                   "For each prominence index, you can select a cicular or level layout."));
    QStringList indicesList;
    indicesList << "None"<< "Random"
                << "Degree Centrality" << "Closeness Centrality"
                << "Influence Range Closeness Centrality"
                << "Betweenness Centrality"
                << "Stress Centrality" << "Eccentricity Centrality"
                << "Power Centrality" << "Information Centrality"
                << "Degree Prestige (inDegree)"  << "PageRank Prestige"
                << "Proximity Prestige";
    toolBoxLayoutByIndexSelect->addItems(indicesList);
    toolBoxLayoutByIndexSelect->setMinimumHeight(20);
    toolBoxLayoutByIndexSelect->setMinimumWidth(120);


    QLabel *toolBoxLayoutByIndexTypeLabel = new QLabel;
    toolBoxLayoutByIndexTypeLabel->setText(tr("Layout Type:"));
    toolBoxLayoutByIndexTypeLabel->setMinimumWidth(10);
    toolBoxLayoutByIndexTypeSelect = new QComboBox;
    toolBoxLayoutByIndexTypeSelect->setStatusTip(
                tr("Select layout type for the selected model"));
    toolBoxLayoutByIndexTypeSelect->setToolTip(
                tr("Select circular or level layout type (you must select an index above)"));
    toolBoxLayoutByIndexTypeSelect->setWhatsThis(
                tr("Layout Type\n\n"
                   "Select a layout type (circular or level) for the selected prominence-based model "
                   "you want to apply to the network.)"));
    QStringList layoutTypes;
    layoutTypes << "Circular" << "On Levels" << "Nodal size";
    toolBoxLayoutByIndexTypeSelect->addItems(layoutTypes);
    toolBoxLayoutByIndexTypeSelect->setMinimumHeight(20);
    toolBoxLayoutByIndexTypeSelect->setMinimumWidth(120);

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
    layoutByIndexGrid -> setSpacing(5);
    layoutByIndexGrid -> setContentsMargins(5, 5, 5, 5);

    //create a box and set the above layout inside
    QGroupBox *layoutByIndexBox= new QGroupBox(tr("By Prominence Index"));
    layoutByIndexBox->setMinimumHeight(120);
    layoutByIndexBox->setLayout (layoutByIndexGrid );


    // create widgets for the "Force-Directed Models" Box
    QLabel *toolBoxLayoutForceDirectedSelectLabel = new QLabel;
    toolBoxLayoutForceDirectedSelectLabel->setText(tr("Model:"));
    toolBoxLayoutForceDirectedSelectLabel->setMinimumWidth(110);
    toolBoxLayoutForceDirectedSelect = new QComboBox;
    QStringList modelsList;
    modelsList << tr("None")
                << tr("Spring Embedder (Eades)")
                << tr("Fruchterman-Reingold")
                << tr("Kamada-Kawai") ;

    toolBoxLayoutForceDirectedSelect->addItems(modelsList);
    toolBoxLayoutForceDirectedSelect->setMinimumHeight(20);
    toolBoxLayoutForceDirectedSelect->setMinimumWidth(120);
    toolBoxLayoutForceDirectedSelect->setStatusTip (
                            tr("Select a Force-Directed layout model. "));
    toolBoxLayoutForceDirectedSelect->setToolTip (
                tr("Select a Force-Directed layout model to embed to the network\n\n"
                   "Available models: \n"
                   "Eades:\n"
                   "A spring-gravitational model, where each node is \n"
                   "regarded as physical object (ring) repeling all other \n"
                   "nodes, while springs between connected nodes attract them. \n\n"

                   "Fruchterman-Reingold: Vertices that are neighbours "
                   "attract each other but, unlike Eades Spring "
                   "Embedder, all vertices repel each other.\n\n"
                   "Kamada-Kawai\n"
                   "Every two vertices are connected  by a 'spring' of a \n"
                   "desirable length, which corresponds to their graph theoretic \n"
                   "distance. In this way, the optimal layout of the graph \n"
                   "is the state with the minimum imbalance. The degree of \n"
                   "imbalance is formulated as the total spring energy: \n"
                   "the square summation of the differences between desirable \n"
                   "distances and real ones for all pairs of vertices"
                   )
                );
    toolBoxLayoutForceDirectedSelect->setWhatsThis(
                tr("Visualize by a Force-Directed layout model.\n\n"
                   "Available models: \n\n "
                   "Eades model\n "
                   "A spring-gravitational model, where each node is \n"
                   "regarded as physical object (ring) repeling all other \n"
                   "nodes, while springs between connected nodes attract them. \n\n"

                   "Fruchterman-Reingold\n"
                   "In this model, the vertices behave as atomic particles \n"
                   "or celestial bodies, exerting attractive and repulsive \n"
                   "forces to each other. Again, only vertices that are \n"
                   "neighbours  attract each other but, unlike Eades Spring \n"
                   "Embedder, all vertices repel each other.\n\n"
                   "Kamada-Kawai\n"
                   "In this model, the graph is considered to be a dynamic system \n"
                   "where every two vertices are connected  by a 'spring' of a \n"
                   "desirable length, which corresponds to their graph theoretic \n"
                   "distance. In this way, the optimal layout of the graph \n"
                   "is the state with the minimum imbalance. The degree of \n"
                   "imbalance is formulated as the total spring energy: \n"
                   "the square summation of the differences between desirable \n"
                   "distances and real ones for all pairs of vertices"
                   )
                );

    toolBoxLayoutForceDirectedButton = new QPushButton(tr("Apply"));
    toolBoxLayoutForceDirectedButton->setFocusPolicy(Qt::NoFocus);
    toolBoxLayoutForceDirectedButton->setMinimumHeight(20);
    toolBoxLayoutForceDirectedButton->setMaximumWidth(60);

    //create layout for dynamic visualisation
    QGridLayout *layoutForceDirectedGrid = new QGridLayout();
    layoutForceDirectedGrid -> addWidget(toolBoxLayoutForceDirectedSelectLabel, 0,0);
    layoutForceDirectedGrid -> addWidget(toolBoxLayoutForceDirectedSelect, 0,1);
    layoutForceDirectedGrid -> addWidget(toolBoxLayoutForceDirectedButton, 1,1);
    layoutForceDirectedGrid -> setSpacing(5);
    layoutForceDirectedGrid -> setContentsMargins(5,5, 5, 5);

    //create a box for dynamic layout options
    QGroupBox *layoutDynamicBox= new QGroupBox(tr("By Force-Directed Model"));
    layoutDynamicBox->setMinimumHeight(90);
    layoutDynamicBox->setLayout (layoutForceDirectedGrid );


    //create widgets for additional visualization options box
    nodeSizesByOutDegreeBx = new QCheckBox(
                tr("Node sizes by OutDegree") );
    nodeSizesByOutDegreeBx ->setEnabled(true);
    nodeSizesByOutDegreeBx
            ->setStatusTip(
                tr("Enable to have all nodes resized so that their "
                   "size reflect their out-degree."));

    nodeSizesByOutDegreeBx
            ->setToolTip(
                tr("If you enable this, all nodes will be resized so that their "
                   "size reflect their out-degree. \n"
                   "Nodes with more directed edges starting from them will be bigger..."));

    nodeSizesByInDegreeBx = new QCheckBox(
                tr("Node sizes by InDegree") );
    nodeSizesByInDegreeBx ->setEnabled(true);
    nodeSizesByInDegreeBx
            ->setStatusTip(
                tr("Enable to have all nodes resized so that their "
                   "size reflect their in-degree." ) );
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
    layoutOptionsGrid->setSpacing(5);
    layoutOptionsGrid->setContentsMargins(5, 5, 5, 5);

    //Box for additional visualization options
    QGroupBox *visualizeOptionsBox= new QGroupBox(tr("Options"));
    visualizeOptionsBox->setMinimumHeight(110);
    visualizeOptionsBox->setMaximumWidth(280);
    visualizeOptionsBox->setLayout (layoutOptionsGrid );


    //Parent box with vertical layout for all layout/visualization boxes
    QVBoxLayout *visualizationBoxLayout = new QVBoxLayout;
    visualizationBoxLayout -> addWidget(layoutByIndexBox);
    visualizationBoxLayout -> addWidget(layoutDynamicBox);
    visualizationBoxLayout -> addWidget(visualizeOptionsBox);

    QGroupBox *visualizationBox= new QGroupBox(tr("Visualize"));
    visualizationBox->setMaximumWidth(280);
    visualizationBox->setLayout (visualizationBoxLayout );

    //Parent box with vertical layout for all boxes of Controls
    QGridLayout *editGrid = new QGridLayout;
    editGrid -> addWidget(editGroupBox, 0,0);
    editGrid -> addWidget(analysisBox, 1, 0);
    editGrid -> addWidget(visualizationBox, 2, 0);
    editGrid -> setRowStretch(3,1);   //fix stretch

    //create a box with title
    leftPanel = new QGroupBox(tr("Control Panel"));
    leftPanel -> setLayout (editGrid);

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

    QLabel *networkLabel = new QLabel;
    networkLabel-> setText ("Network");
    networkLabel ->setFont(QFont("sans-serif", 10, QFont::Bold));
    propertiesGrid -> addWidget(networkLabel , 0,0);
    propertiesGrid -> addWidget(labelNodesLCD, 1,0);
    propertiesGrid -> addWidget(nodesLCD,1,1);
    propertiesGrid -> addWidget(labelEdgesLCD, 2,0);
    propertiesGrid -> addWidget(edgesLCD,2,1);
    propertiesGrid -> addWidget(labelDensityLCD, 3,0);
    propertiesGrid -> addWidget(densityLCD,3,1);

    QLabel *dummyLabel = new QLabel;
    dummyLabel-> setText ("");
    QLabel *labelNode = new QLabel;
    labelNode-> setText (tr("Active Node"));
    labelNode ->setFont(QFont("sans-serif", 10, QFont::Bold));

    QLabel *labelSelectedNodeLCD = new QLabel;
    labelSelectedNodeLCD -> setText (tr("Number:"));
    labelSelectedNodeLCD -> setToolTip (tr("This is the number of the last selected node."));

    selectedNodeLCD =new QLCDNumber(5);
    selectedNodeLCD ->setSegmentStyle(QLCDNumber::Flat);

    QLabel *labelInDegreeLCD = new QLabel;
    labelInDegreeLCD -> setText (tr("In-Degree:"));
    labelInDegreeLCD -> setToolTip (tr("The sum of all in-edge weights of the node you clicked.."));
    inDegreeLCD=new QLCDNumber(5);
    inDegreeLCD -> setSegmentStyle(QLCDNumber::Flat);
    inDegreeLCD -> setToolTip (tr("The sum of all in-edge weights of the node you clicked."));
    QLabel *labelOutDegreeLCD = new QLabel;
    labelOutDegreeLCD -> setText (tr("Out-Degree:"));
    labelOutDegreeLCD -> setToolTip (tr("The sum of all out-edge weights of the node you clicked."));
    outDegreeLCD=new QLCDNumber(5);
    outDegreeLCD -> setSegmentStyle(QLCDNumber::Flat);
    outDegreeLCD -> setToolTip (tr("The sum of all out-edge weights of the node you clicked."));

    QLabel *labelClucofLCD  = new QLabel;
    labelClucofLCD -> setText (tr("Clu.Coef."));
    labelClucofLCD -> setToolTip (tr("The Clustering Coefficient quantifies how close the clicked vertex and its neighbors are to being a clique. \nThe value is the proportion of Edges between the vertices within the neighbourhood of the clicked vertex,\n divided by the number of Edges that could possibly exist between them. \n\n WARNING: This value is automatically calculated only if vertices < 500.\n If your network is larger than 500 vertices, compute CluCof from the menu Analysis > Clustering Coefficient "));
    clucofLCD = new QLCDNumber(5);
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

    //create a panel with title
    rightPanel = new QGroupBox(tr("Statistics Panel"));
    rightPanel -> setLayout (propertiesGrid);

}









/**
 * @brief MainWindow::initStatusBar
 * Initializes the status bar
 */
void MainWindow::initStatusBar() {
    statusBarDuration=3000;
    statusMessage( tr("Ready."));
}





/**
 * @brief MainWindow::initView
 * Initializes the scene and the corresponding graphicsWidget,
 * The latter is a QGraphicsView canvas which is the main widget of SocNetV.
 */
void MainWindow::initView() {
    qDebug ("MW initView()");
    //create a scene
    scene=new QGraphicsScene();

    //create a view widget for this scene
    graphicsWidget=new GraphicsWidget(scene, this);
    graphicsWidget->setViewportUpdateMode( QGraphicsView::SmartViewportUpdate );
    //  FullViewportUpdate  // MinimalViewportUpdate //SmartViewportUpdate  //BoundingRectViewportUpdate
    //QGraphicsView can cache pre-rendered content in a QPixmap, which is then drawn onto the viewport.
    graphicsWidget->setCacheMode(QGraphicsView::CacheNone);  //CacheBackground | CacheNone

    bool antialiasing = (appSettings["antialiasing"] == "true" ) ? true:false;
    graphicsWidget->setRenderHint(QPainter::Antialiasing, antialiasing );
    graphicsWidget->setRenderHint(
                QPainter::TextAntialiasing, antialiasing );
    graphicsWidget->setRenderHint(QPainter::SmoothPixmapTransform, antialiasing );
    //Optimization flags:
    //if items do restore their state, it's not needed for graphicsWidget to do the same...
    graphicsWidget->setOptimizationFlag(QGraphicsView::DontSavePainterState, true);
    //Disables QGraphicsView's antialiasing auto-adjustment of exposed areas.
    graphicsWidget->setOptimizationFlag(QGraphicsView::DontAdjustForAntialiasing, false);
    //"QGraphicsScene applies an indexing algorithm to the scene, to speed up item discovery functions like items() and itemAt().
    // Indexing is most efficient for static scenes (i.e., where items don't move around).
    // For dynamic scenes, or scenes with many animated items, the index bookkeeping can outweight the fast lookup speeds." So...
    scene->setItemIndexMethod(QGraphicsScene::BspTreeIndex); //NoIndex (for anime) | BspTreeIndex

    graphicsWidget->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    //graphicsWidget->setTransformationAnchor(QGraphicsView::AnchorViewCenter);
    //graphicsWidget->setTransformationAnchor(QGraphicsView::NoAnchor);
    graphicsWidget->setResizeAnchor(QGraphicsView::AnchorViewCenter);

    // sets dragging the mouse over the scene while the left mouse button is pressed.
    graphicsWidget->setDragMode(QGraphicsView::RubberBandDrag);
    graphicsWidget->setFocusPolicy(Qt::StrongFocus);
    graphicsWidget->setFocus();
    graphicsWidget->setWhatsThis(tr("The canvas of SocNetV. \n\n"
                                  "Inside this area you create and edit networks, "
                                  "load networks from files and visualize them \n"
                                  "according to selected metrics. \n\n"
                                  " - To create a new node, double-click anywhere (Ctrl+X+A)\n"
                                  " - To add an arc between two nodes, double-click"
                                  " on the first node then double-click on the second (Ctrl+E+A)\n"
                                  " - To change network appearance, right click on empty space\n"
                                  " - To change/edit the properties of a node, right-click on it\n"
                                  " - To change/edit the properties of an edge, right-click on it."
                                  ""));

}




/**
 * @brief MainWindow::initWindowLayout
 * Initializes the application window UI:
 * Creates helper widgets and sets the main layout of the MainWindow
 */
void MainWindow::initWindowLayout() {
    qDebug () << "MW::initWindowLayout";
    int size = style()->pixelMetric(QStyle::PM_ToolBarIconSize);
    QSize iconSize(size, size);
    iconSize.setHeight(16);
    iconSize.setWidth(16);
    // Zoom slider
    zoomInBtn = new QToolButton;
    zoomInBtn->setShortcut(Qt::CTRL + Qt::Key_Plus);
    zoomInBtn->setToolTip(tr("Zoom in (Ctrl++)"));
    zoomInBtn->setStatusTip(tr("Zoom inside the actual network. Or press Cltr and use mouse wheel."));
    zoomInBtn->setWhatsThis(tr("Zoom In.\n\n"
                               "Zooms in the actual network"
                               "You can also press Cltr and use mouse wheel."));
    zoomInBtn->setAutoRepeat(true);
    zoomInBtn->setAutoRepeatInterval(33);
    zoomInBtn->setAutoRepeatDelay(0);
    zoomInBtn->setIcon(QPixmap(":/images/zoomin.png"));
    zoomInBtn->setIconSize(iconSize);

    zoomOutBtn = new QToolButton;
    zoomOutBtn->setAutoRepeat(true);
    zoomOutBtn->setShortcut(Qt::CTRL + Qt::Key_Minus);
    zoomOutBtn->setToolTip(tr("Zoom out (Ctrl+-)"));
    zoomOutBtn->setStatusTip(tr("Zoom out of the actual network. Or press Cltr and use mouse wheel."));
    zoomOutBtn->setWhatsThis(tr("Zoom out.\n\n"
                                "Zooms out the actual network"
                                "You can also press Cltr and use mouse wheel."));
    zoomOutBtn->setAutoRepeat(true);
    zoomOutBtn->setAutoRepeatInterval(33);
    zoomOutBtn->setAutoRepeatDelay(0);
    zoomOutBtn->setIcon(QPixmap(":/images/zoomout.png"));
    zoomOutBtn->setIconSize(iconSize);

    zoomSlider = new QSlider;
    zoomSlider->setMinimum(0);
    zoomSlider->setMaximum(500);
    zoomSlider->setValue(250);
    zoomSlider->setToolTip(tr("Zoom slider: Drag up to zoom in. \n"
                                "Drag down to zoom out. "));
    zoomSlider->setWhatsThis(tr("Zoom slider: Drag up to zoom in. \n"
                                "Drag down to zoom out. "));
    zoomSlider->setTickPosition(QSlider::TicksBothSides);

    // Zoom slider layout
    QVBoxLayout *zoomSliderLayout = new QVBoxLayout;
    zoomSliderLayout->addWidget(zoomInBtn);
    zoomSliderLayout->addWidget(zoomSlider);
    zoomSliderLayout->addWidget(zoomOutBtn);

    // Rotate slider
    rotateLeftBtn = new QToolButton;
    rotateLeftBtn->setAutoRepeat(true);
    rotateLeftBtn->setShortcut(Qt::CTRL + Qt::Key_Left);
    rotateLeftBtn->setIcon(QPixmap(":/images/rotateleft.png"));
    rotateLeftBtn->setToolTip(tr("Rotate counterclockwise (Ctrl+Left Arrow)"));
    rotateLeftBtn->setStatusTip(tr("Rotate counterclockwise (Ctrl+Left Arrow)"));
    rotateLeftBtn->setWhatsThis(tr("Rotates counterclockwise (Ctrl+Left Arrow)"));
    rotateLeftBtn->setIconSize(iconSize);

    rotateRightBtn = new QToolButton;
    rotateRightBtn->setAutoRepeat(true);
    rotateRightBtn->setShortcut(Qt::CTRL + Qt::Key_Right);
    rotateRightBtn ->setIcon(QPixmap(":/images/rotateright.png"));
    rotateRightBtn->setToolTip(tr("Rotate clockwise (Ctrl+Right Arrow)"));
    rotateRightBtn->setStatusTip(tr("Rotate clockwise (Ctrl+Right Arrow)"));
    rotateRightBtn->setWhatsThis(tr("Rotates clockwise (Ctrl+Right Arrow)"));
    rotateRightBtn ->setIconSize(iconSize);

    rotateSlider = new QSlider;
    rotateSlider->setOrientation(Qt::Horizontal);
    rotateSlider->setMinimum(-180);
    rotateSlider->setMaximum(180);
    rotateSlider->setTickInterval(5);
    rotateSlider->setValue(0);
    rotateSlider->setToolTip(tr("Rotate slider: Drag to left to rotate clockwise. \n"
                                "Drag to right to rotate counterclockwise. "));
    rotateSlider->setWhatsThis(tr("Rotate slider: Drag to left to rotate clockwise. "
                                "Drag to right to rotate counterclockwise. "));
    rotateSlider->setTickPosition(QSlider::TicksBothSides);

    // Rotate slider layout
    QHBoxLayout *rotateSliderLayout = new QHBoxLayout;
    rotateSliderLayout->addWidget(rotateLeftBtn);
    rotateSliderLayout->addWidget(rotateSlider);
    rotateSliderLayout->addWidget(rotateRightBtn );

    resetSlidersBtn = new QToolButton;
    resetSlidersBtn->setText(tr("Reset"));
    resetSlidersBtn->setShortcut(Qt::CTRL + Qt::Key_0);
    resetSlidersBtn->setToolTip(tr("Reset zoom and rotation to zero (Ctrl+0)"));
    resetSlidersBtn->setWhatsThis(tr("Reset zoom and rotation to zero (Ctrl+0)"));
    resetSlidersBtn->setIcon(QPixmap(":/images/reset.png"));
    resetSlidersBtn ->setIconSize(iconSize);
    resetSlidersBtn->setEnabled(true);

    // Create a layout for the toolbox and the canvas.
    // This will be the layout of our MW central widget
    QGridLayout *layout = new QGridLayout;
    layout->addWidget(leftPanel, 0, 0, 2,1);
    layout->addWidget(graphicsWidget,0,1);
    layout->addLayout(zoomSliderLayout, 0, 2);
    layout->addWidget(rightPanel, 0, 3,2,1);
    layout->addLayout(rotateSliderLayout, 1, 1, 1, 1);
    layout->addWidget(resetSlidersBtn, 1, 2, 1, 1);

    //create a dummy widget, and set the above layout
    QWidget *widget = new QWidget;
    widget->setLayout(layout);

    //now set this as central widget of MW
    setCentralWidget(widget);

    // set panels visibility
    if ( appSettings["showRightPanel"] == "false") {
        slotOptionsRightPanelVisibility(false);
    }

    if ( appSettings["showLeftPanel"] == "false") {
        slotOptionsLeftPanelVisibility(false);
    }

    qDebug () << "MW::initWindowLayout - resize to 1280x900";
    this->resize(1280,900);

    this->showMaximized();

}







/**
 * @brief MainWindow::initSignalSlots
 * Connect signals & slots between various parts of the app:
 * - the GraphicsWidget and the Graph
 * - the GraphicsWidget and the MainWindow
 * This must be called after all widgets have been created.
 *
 */
void MainWindow::initSignalSlots() {

    // Signals from graphicsWidget to MainWindow

    connect( graphicsWidget, SIGNAL( resized(int, int)),
                &activeGraph, SLOT( setMaximumSize(int,int)) ) ;

    connect( graphicsWidget, SIGNAL( selectedNode(Node*) ),
             this, SLOT( nodeInfoStatusBar(Node*) ) 	);

    connect( graphicsWidget, SIGNAL( selectedEdge(Edge*) ),
             this, SLOT ( edgeInfoStatusBar(Edge*) )  );



    connect( graphicsWidget, SIGNAL( userClickOnEmptySpace() ),
                     this, SLOT( slotEditClickOnEmptySpace() ) ) ;

    connect( graphicsWidget, SIGNAL( userDoubleClicked(int, QPointF) ),
             this, SLOT( slotEditNodeAddWithMouse(int,QPointF) ) ) ;

    connect( graphicsWidget, SIGNAL( userMiddleClicked(int, int, float) ),
             this, SLOT( slotEditEdgeCreate(int, int, float) ) 	);


    connect( graphicsWidget, SIGNAL( openNodeMenu() ),
             this, SLOT( slotEditNodeOpenContextMenu() ) ) ;

    connect( graphicsWidget, SIGNAL( openEdgeMenu() ),
             this, SLOT( openEdgeContextMenu() ) ) ;

    connect (graphicsWidget, &GraphicsWidget::openContextMenu,
             this, &MainWindow::slotEditOpenContextMenu);

    connect( graphicsWidget, SIGNAL(updateNodeCoords(int, int, int)),
             this, SLOT( updateNodeCoords(int, int, int) ) );


    connect( graphicsWidget, SIGNAL(zoomChanged(const int &)),
             zoomSlider, SLOT( setValue(const int &)) );

    connect(zoomSlider, SIGNAL(valueChanged(const int &)),
            graphicsWidget, SLOT(changeMatrixScale(const int &)));

    connect( zoomInBtn, SIGNAL(clicked()), graphicsWidget, SLOT( zoomIn() ) );
    connect( zoomOutBtn, SIGNAL(clicked()), graphicsWidget, SLOT( zoomOut() ) );

    connect( graphicsWidget, SIGNAL(rotationChanged(const int &)),
             rotateSlider, SLOT( setValue(const int &)) );

    connect(rotateSlider, SIGNAL(valueChanged(const int &)),
            graphicsWidget, SLOT(changeMatrixRotation(const int &)));

    connect(rotateLeftBtn, SIGNAL(clicked()), graphicsWidget, SLOT(rotateLeft()));
    connect(rotateRightBtn, SIGNAL(clicked()), graphicsWidget, SLOT(rotateRight()));

    connect(resetSlidersBtn, SIGNAL(clicked()), graphicsWidget, SLOT(reset()));


    // Signals from activeGraph to graphicsWidget
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
             this, SLOT( slotNetworkChanged() ) ) ;

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


    connect( &activeGraph, SIGNAL( changeEdgeColor(long int,long int,QString)),
             graphicsWidget, SLOT( setEdgeColor(long int,long int,QString) ) );



    connect( &activeGraph, SIGNAL( eraseNode(long int) ),
             graphicsWidget, SLOT(  eraseNode(long int) ) );

    connect( &activeGraph, SIGNAL( setEdgeVisibility (int, int, int, bool) ),
             graphicsWidget, SLOT(  setEdgeVisibility (int, int, int, bool) ) );

    connect( &activeGraph, SIGNAL( setVertexVisibility(long int, bool)  ),
             graphicsWidget, SLOT(  setNodeVisibility (long int ,  bool) ) );

    connect( &activeGraph, SIGNAL( setNodeSize(long int, int)  ),
             graphicsWidget, SLOT(  setNodeSize (long int , int) ) );

    connect( &activeGraph, SIGNAL( setNodeColor(long int,QString))  ,
             graphicsWidget, SLOT(  setNodeColor(long int, QString) ) );

    connect( &activeGraph, SIGNAL( setNodeShape(long int,QString))  ,
             graphicsWidget, SLOT(  setNodeShape(long int, QString) ) );


    connect( &activeGraph, &Graph::setNodeLabel ,
             graphicsWidget, &GraphicsWidget::setNodeLabel );


    connect( &activeGraph, SIGNAL( statusMessage (QString) ),
             this, SLOT( statusMessage (QString) ) ) ;

    connect( &activeGraph, SIGNAL( describeDataset (QString) ),
             this, SLOT( showMessageToUser (QString) ) ) ;

    connect( &activeGraph, &Graph::signalNodeSizesByInDegree,
             this, &MainWindow::slotLayoutNodeSizesByInDegree );




    //signals and slots inside MainWindow
    connect( editNodeAddBt,SIGNAL(clicked()), this, SLOT( slotEditNodeAdd() ) );

    connect( editEdgeAddBt,SIGNAL(clicked()), this, SLOT( slotEditEdgeAdd() ) );

    connect( removeNodeBt,SIGNAL(clicked()), this, SLOT( slotEditNodeRemove() ) );

    connect( editEdgeRemoveBt,SIGNAL(clicked()), this, SLOT( slotEditEdgeRemove() ) );


    connect( editRelationNextAct, SIGNAL(triggered()),
             this, SLOT( slotEditRelationNext() ) );
    connect( editRelationPreviousAct, SIGNAL(triggered()),
             this, SLOT( slotEditRelationPrev() ) );
    connect( editRelationAddAct, SIGNAL(triggered()), this, SLOT( slotEditRelationAdd() ) );

    connect( editRelationChangeCombo , SIGNAL( currentIndexChanged(int) ) ,
             &activeGraph, SLOT( changeRelation(int) ) );

    connect( this , SIGNAL(addRelationToGraph(QString)),
             &activeGraph, SLOT( addRelationFromUser(QString) ) );

    connect ( &activeGraph, SIGNAL(addRelationToMW(QString)),
              this, SLOT(slotEditRelationAdd(QString)));

    connect( &activeGraph, SIGNAL(relationChanged(int)),
             graphicsWidget, SLOT( changeRelation(int))  ) ;

    connect( &m_filterEdgesByWeightDialog, SIGNAL( userChoices( float, bool) ),
             &activeGraph, SLOT( filterEdgesByWeight (float, bool) ) );


    connect( &m_WebCrawlerDialog, &WebCrawlerDialog::userChoices,
             this, &MainWindow::slotNetworkWebCrawler );

    connect( &m_datasetSelectDialog, SIGNAL( userChoices( QString) ),
             this, SLOT( slotNetworkDataSetRecreate(QString) ) );

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

    connect(toolBoxLayoutForceDirectedButton, SIGNAL (clicked() ),
            this, SLOT(toolBoxLayoutForceDirectedButtonPressed() ) );

    connect( layoutGuidesBx, SIGNAL(stateChanged(int)),
             this, SLOT(slotLayoutGuides(int)));

}







/**
 * @brief MainWindow::initNet
 * Initializes the default network parameters.
 * Used on app start and especially when erasing a network to start a new one
 */
void MainWindow::initNet(){
    qDebug()<<"MW: initNet() START INITIALISATION";
    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

    // Init basic variables


    considerWeights=false;
    inverseWeights=false;
    askedAboutWeights=false;


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
    networkSave->setIcon(QIcon(":/images/saved.png"));
    networkSave->setEnabled(true);

    markedNodesExist=false;	//used by slotEditNodeFind()

    cursorPosGW=QPointF(-1,-1);
    clickedJimNumber=-1;
    edgeClicked=false;
    nodeClicked=false;


    /** Clear previous network data */
    activeGraph.clear();
    activeGraph.setSocNetV_Version(VERSION);

    activeGraph.setInitVertexShape(appSettings["initNodeShape"]);
    activeGraph.setInitVertexSize(appSettings["initNodeSize"].toInt(0, 10));
    activeGraph.setInitVertexColor( appSettings["initNodeColor"] );

    activeGraph.setInitVertexNumberSize(appSettings["initNumberSize"].toInt(0,10));
    activeGraph.setInitVertexNumberColor(appSettings["initNumberColor"]);

    activeGraph.setInitVertexLabelColor(appSettings["initLabelColor"]);
    activeGraph.setInitVertexLabelSize(appSettings["initLabelSize"].toInt(0,10));

    activeGraph.setInitEdgeColor(appSettings["initEdgeColor"]);

    activeGraph.setShowLabels(
                (appSettings["initNodeLabelsVisibility"] == "true" ) ? true: false
                );
    activeGraph.setShowNumbersInsideNodes(
                ( appSettings["initNodeNumbersInside"] == "true" ) ? true: false
                );

    /** Clear graphicsWidget scene and reset transformations **/
    graphicsWidget->clear();
    rotateSlider->setValue(0);
    zoomSlider->setValue(250);

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
    toolBoxLayoutForceDirectedSelect->setCurrentIndex(0);
    nodeSizesByOutDegreeBx->setChecked(false);
    nodeSizesByInDegreeBx->setChecked(false);
    displayEdgesWeightNumbersAct->setChecked(false);
    considerEdgeWeightsAct->setChecked(false);
    //displayEdgesArrowsAct->setChecked(false);		//FIXME: USER PREFS EMITTED TO GRAPH?

    filterIsolateNodesAct->setChecked(false); // re-init orphan nodes menu item

    editRelationChangeCombo->clear();

    graphicsWidget->setInitNodeColor(appSettings["initNodeColor"]);
    graphicsWidget->setInitNumberDistance(numberDistance);
    graphicsWidget->setInitLabelDistance(labelDistance);
    graphicsWidget->setInitZoomIndex(250);
    graphicsWidget->setInitNodeSize(appSettings["initNodeSize"].toInt(0, 10));
    if (appSettings["initBackgroundImage"] != ""
            && QFileInfo(appSettings["initBackgroundImage"]).exists()) {
        graphicsWidget->setBackgroundBrush(QImage(appSettings["initBackgroundImage"]));
        graphicsWidget->setCacheMode(QGraphicsView::CacheBackground);
        statusMessage( tr("BackgroundImage on.") );
    }
    else {
        graphicsWidget->setBackgroundBrush(
                    QBrush(QColor (appSettings["initBackgroundColor"]))
                    ); //Qt::gray
    }




    /** set window title **/
    setWindowTitle(tr("Social Network Visualizer ")+VERSION);

    QApplication::restoreOverrideCursor();
    statusMessage( tr("Ready"));
    qDebug("MW: initNet() INITIALISATION END");


}





/**
 * @brief MainWindow::statusMessage
 * @param message
 * Convenience method to show a message in the status bar, with the given duration
 * Slot called by Graph::statusMessage to display some message to the user
 */
void MainWindow::statusMessage(const QString message){
    statusBar()->showMessage( message, statusBarDuration );
}



/**
 * @brief MainWindow::showMessageToUser
 * @param message
 * Convenience method
 */
void MainWindow::showMessageToUser(const QString message) {
    QMessageBox::information(this, tr("Info"),
                          message,
                          QMessageBox::Ok, 0);
}




/**
 * @brief MainWindow::updateNodeCoords
 * @param nodeNumber
 * @param x
 * @param y
 * Called from GraphicsWidget when a node moves to update vertex coordinates
 *  in Graph
 */
void MainWindow::updateNodeCoords(int nodeNumber, int x, int y){
    //	qDebug("MW: updateNodeCoords() for %i with x %i and y %i", nodeNumber, x, y);
    activeGraph.updateVertCoords(nodeNumber, x, y);
}





/**
 * @brief MainWindow::toolBoxAnalysisGeodesicsSelectChanged
 * @param selectedIndex
 * Called from MW, when user selects something in the Geodesics selectbox of
 *  toolbox
 */
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





/**
 * @brief MainWindow::toolBoxAnalysisConnectivitySelectChanged
 * @param selectedIndex
 * Called from MW, when user selects something in the Connectivity selectbox of
 *  toolbox
 */
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




/**
 * @brief MainWindow::toolBoxAnalysisClusterabilitySelectChanged
 * @param selectedIndex
 * Called from MW, when user selects something in the Clusterability selectbox
 * of toolbox
 */
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





/**
 * @brief MainWindow::toolBoxAnalysisProminenceSelectChanged
 * @param selectedIndex
 * Called from MW, when user selects something in the Prominence selectbox
 *  of toolbox
 */
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

/**
 * @brief MainWindow::toolBoxLayoutByIndexButtonPressed
 * Called from MW, when user selects an index in the Layout by index selectbox
 *  of the left panel.
 */
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



/**
 * @brief MainWindow::toolBoxLayoutForceDirectedButtonPressed
 * Called from MW, when user selects a model in the Layout by Force Directed
 * selectbox of left panel.
 */
void MainWindow::toolBoxLayoutForceDirectedButtonPressed(){
    qDebug()<<"MW::toolBoxLayoutForceDirectedButtonPressed()";
    int selectedModel = toolBoxLayoutForceDirectedSelect->currentIndex();
    QString selectedModelText = toolBoxLayoutForceDirectedSelect -> currentText();
    qDebug() << " selected index is " << selectedModelText << " : "
             << selectedModel;

    switch(selectedModel) {
    case 0:
        break;
    case 1:
        slotLayoutSpringEmbedder();
        break;
    case 2:
        slotLayoutFruchterman();
        break;
    default:
        toolBoxLayoutForceDirectedSelect->setCurrentIndex(0);
        break;
    };
}






/**
 * @brief MainWindow::resizeEvent
 * Resizes the scene when the window is resized.
 */
void MainWindow::resizeEvent( QResizeEvent * ){

    qDebug ("MW resizeEvent():  window size %i, %i, graphicsWidget size %i, %i, scene %f,%f",
            width(),height(),
            graphicsWidget->width(),graphicsWidget->height(),
            graphicsWidget->scene()->width(), graphicsWidget->scene()->height());
    statusMessage(
                QString(
                    tr("Window resized to (%1, %2)px. Canvas size: (%3, %4) px"))
                .arg(width()).arg(height())
                .arg(graphicsWidget->width()).arg(graphicsWidget->height())
                );

}




/**
 * @brief MainWindow::closeEvent
 * @param ce
 * Closes the application. Asks to write any unsaved network data.
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
        slotNetworkSave();
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
 * @brief MainWindow::slotNetworkNew
 * Creates a new network
 */
void MainWindow::slotNetworkNew() {
    slotNetworkClose();
}



/**
 * @brief MainWindow::getLastPath
 * returns the last path used by user to open/save something
 */
QString MainWindow::getLastPath() {
    if ( appSettings["lastUsedDirPath"] == "socnetv-initial-none") {
        appSettings["lastUsedDirPath"] = appSettings["dataDir"];
    }
    qDebug() << appSettings["lastUsedDirPath"] ;
    return appSettings["lastUsedDirPath"] ;
}


/**
 * @brief MainWindow::setLastPath
 * sets the last path used by user to open/save something
 * @param filePath
 */
void MainWindow::setLastPath(QString filePath) {
    appSettings["lastUsedDirPath"] = filePath.left( filePath.lastIndexOf("/"));
    qDebug() << appSettings["lastUsedDirPath"];
}


/**
 * @brief MainWindow::slotNetworkFileChoose
 * Prompts the user a directory dialogue to choose a file from.
 * Calls slotNetworkFilePreview()
 */
void MainWindow::slotNetworkFileChoose() {

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

        slotNetworkFilePreview(m_fileName, m_fileFormat );

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
 * @brief MainWindow::slotNetworkSave
 * Saves the network in the same file
 */
void MainWindow::slotNetworkSave() {
    statusMessage( tr("Saving file..."));

    if (!fileLoaded && !networkModified ) {
        statusMessage(  QString(tr("No network loaded.")) );
        return;
    }
    if ( fileName.isEmpty() ) {
        slotNetworkSaveAs();
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
 * @brief MainWindow::slotNetworkSaveAs
 * Saves the network in a new file
 */
void MainWindow::slotNetworkSaveAs() {
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
        slotNetworkSave();
    }
    else  {
        statusMessage( tr("Saving aborted"));
        return;
    }
    statusMessage( tr("Ready."));
}



/**
 * @brief MainWindow::networkSaved
 * @param saved_ok
 * Called from Graph when we try to save file
 */
void MainWindow::networkSaved(int saved_ok)
{
    if (saved_ok <= 0)
    {
        slotNetworkChanged();
        statusMessage( tr("Error! Could not save this file... ")+fileNameNoPath.last()+tr(".") );
    }
    else
    {
        networkSave->setIcon(QIcon(":/images/saved.png"));
        networkSave->setEnabled(false);
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
 * @brief MainWindow::slotNetworkClose
 * Closes the network. Saves it if necessary. Used by createNew.
 */
void MainWindow::slotNetworkClose() {
    qDebug()<<"slotNetworkClose()";
    statusMessage( tr("Closing network file..."));
    if (networkModified) {
        switch ( QMessageBox::information (this,
                                           "Closing Network...",
                                           tr("Network has not been saved. \nDo you want to save before closing it?"),
                                           "Yes", "No",0,1))
        {
        case 0: slotNetworkSave(); break;
        case 1: break;
        }
    }
    statusMessage( tr("Erasing old network data...."));
    initNet();
    statusMessage( tr("Ready."));
}



/**
 * @brief MainWindow::slotNetworkPrint
 * Sends the active network to the printer
 */
void MainWindow::slotNetworkPrint() {
    statusMessage( tr("Printing..."));
    QPrintDialog dialog(printer, this);
    if ( dialog.exec() )   {
        QPainter painter(printer);
        graphicsWidget->render(&painter);
    };
    statusMessage( tr("Ready."));
}





/**
 * @brief MainWindow::slotNetworkImportGraphML
 * Imports a network from a GraphML formatted file
 */
void MainWindow::slotNetworkImportGraphML(){
//    fileFormat=-1;
//    checkSelectFileType = true;
    this->slotNetworkFileChoose();
}




/**
 * @brief MainWindow::slotNetworkImportPajek
 * Imports a network from a Pajek-like formatted file
 */
void MainWindow::slotNetworkImportPajek(){
    fileFormat=2;
    checkSelectFileType = false;
    this->slotNetworkFileChoose();
}




/**
 * @brief MainWindow::slotNetworkImportSM
 * Imports a network from a Adjacency matrix formatted file
 */
void MainWindow::slotNetworkImportSM(){
    fileFormat=3;
    checkSelectFileType = false;
    this->slotNetworkFileChoose();
}




/**
 * @brief MainWindow::slotNetworkImportTwoModeSM
 * Imports a network from a two mode sociomatrix formatted file
 */
void MainWindow::slotNetworkImportTwoModeSM(){
    fileFormat=9;
    checkSelectFileType = false;
    this->slotNetworkFileChoose();
}



/**
 * @brief MainWindow::slotNetworkImportDot
 * Imports a network from a Dot formatted file
 */
void MainWindow::slotNetworkImportDot(){
    fileFormat=4;
    checkSelectFileType = false;
    this->slotNetworkFileChoose();
}



/**
 * @brief MainWindow::slotNetworkImportGML
 * Imports a network from a GML formatted file
 */
void MainWindow::slotNetworkImportGML(){
    fileFormat=5;
    checkSelectFileType = false;
    this->slotNetworkFileChoose();
}




/**
 * @brief MainWindow::slotNetworkImportDL
 * Imports a network from a UCINET formatted file
 */
void MainWindow::slotNetworkImportDL(){
    fileFormat=6;
    checkSelectFileType = false;
    this->slotNetworkFileChoose();
}



/**
 * @brief MainWindow::slotNetworkImportEdgeList
 * Imports a network from a List formatted file
 */
void MainWindow::slotNetworkImportEdgeList(){
    switch( QMessageBox::question( this, "Type of list format",
                                   tr("I can parse two kinds of lists: \n\n")+
                                   tr("A. Weighted lists, with each line having exactly 3 columns (source, target, weight), i.e.\n  1 2 5 \n \n")+
                                   tr("B. Simple edge lists, with each line having 2 or more columns (source, target1, target2, ... etc)\n\n")+
                                   tr("Please select the appropriate type of list format for the file you want to load:"),
                                   "Weighted", "Simple",0,1 )
            )
    {
    case 0:
        qDebug() << "***  MW::slotNetworkImportEdgeList - Weighted list selected! " ;
        fileFormat  = 7;
        break;
    case 1:
        qDebug() << "***  MW: slotNetworkImportEdgeList - Simple list selected! " ;
        fileFormat = 8;
        break;
    }
    checkSelectFileType = false;
    this->slotNetworkFileChoose();
}


/**
 * @brief MainWindow::slotNetworkAvailableTextCodecs
 * Setup a list of all text codecs supported by current OS
 */
void MainWindow::slotNetworkAvailableTextCodecs()
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


/**
 * @brief MainWindow::slotNetworkFilePreview
 * @param m_fileName
 * @param m_fileFormat
 * @return
 * Called from slotNetworkFileChoose()
 * Opens a window to preview the selected file where the user
 * can select an appropriate text codec
 */
bool MainWindow::slotNetworkFilePreview(const QString m_fileName,
                                    const int m_fileFormat ){
    qDebug() << "MW::slotNetworkFilePreview() : "<< m_fileName;

    if (!m_fileName.isEmpty()) {
        QFile file(m_fileName);
        if (!file.open(QFile::ReadOnly)) {
            QMessageBox::warning(this, tr("Network File Previewer"),
                                 tr("Cannot read file %1:\n%2")
                                 .arg(m_fileName)
                                 .arg(file.errorString()));
            return false;
        }
        qDebug() << "MW::slotNetworkFilePreview() reading the file now... " ;
        QByteArray data = file.readAll();

        previewForm->setEncodedData(data,m_fileName, m_fileFormat);
        previewForm->exec();
    }
    return true;
}



/**
 * @brief MainWindow::slotNetworkFileLoad
 * @param m_fileName
 * @param m_codecName
 * @param m_fileFormat
 * @return
 * Main network file loader method
 * Called from previewForm and slotNetworkDataSetRecreate
 * Calls initNet to init to default values.
 * Then calls activeGraph::loadGraph to actually load the network...
 */
bool MainWindow::slotNetworkFileLoad(const QString m_fileName,
                                 const QString m_codecName,
                                 const int m_fileFormat )
{
    qDebug() << "MW::slotNetworkFileLoad() : "<< m_fileName
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
    qDebug() << "MW::slotNetworkFileLoad() : calling activeGraph.loadGraph() ";
    bool loadGraphStatus = activeGraph.loadGraph (
                m_fileName,
                m_codecName,
                (appSettings["initNodeLabelsVisibility"] == "true" ) ? true: false,
                graphicsWidget->width(),
                graphicsWidget->height(),
                m_fileFormat, two_sm_mode
                );
    qDebug() << "MW::slotNetworkFileLoad() : loadGraphStatus " << loadGraphStatus;
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
    qDebug() << "MW::slotNetworkFileLoad() : returning " << loadGraphStatus;
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
    slotNetworkChanged();
    networkSave->setIcon(QIcon(":/images/saved.png"));
    networkSave->setEnabled(false);
}


/**
 * @brief MainWindow::slotEditRelationPrev
 * Decreases the index of editRelationChangeCombo
 * which signals to Graph::changeRelation()
 */
void MainWindow::slotEditRelationPrev(){
    qDebug() << "MW::slotEditRelationPrev()";
    int index=editRelationChangeCombo->currentIndex();
    if (index>0){
        --index;
        filterIsolateNodesAct->setChecked(false);
        editRelationChangeCombo->setCurrentIndex(index);
    }
}

/**
 * @brief MainWindow::slotEditRelationNext
 * Increases the index of editRelationChangeCombo
 * which signals to Graph::changeRelation()
 */
void MainWindow::slotEditRelationNext(){
    qDebug() << "MW::slotEditRelationNext()";
    int index=editRelationChangeCombo->currentIndex();
    int relationsCounter=editRelationChangeCombo->count();
    if (index< (relationsCounter -1 )){
        ++index;
        filterIsolateNodesAct->setChecked(false);
        editRelationChangeCombo->setCurrentIndex(index);
    }

}



/**
 * @brief MainWindow::slotEditRelationAdd
 * called from activeGraph::addRelationFromGraph(QString) when the parser or a
 * Graph method demands a new relation to be added in the Combobox.
 * @param relationName (NULL)
 */
void MainWindow::slotEditRelationAdd(QString relationName){
    qDebug() << "MW::slotEditRelationAdd(string)" << relationName;
    if ( !relationName.isNull() ){
        editRelationChangeCombo->addItem(relationName);
    }
}

/**
 * @brief MainWindow::slotEditRelationAdd
 * Called from MW when user clicks New Relation btn
 * or when the user creates the first edge visually.
 */
void MainWindow::slotEditRelationAdd(){
    qDebug() << "MW::slotEditRelationAdd()";
    bool ok;
    QString newRelationName;
    int relationsCounter=editRelationChangeCombo->count();
    if (relationsCounter==0) {
        newRelationName = QInputDialog::getText(
                    this,
                    tr("Add new relation"),
                    tr("Enter a name for this new relation between the actors.\n"
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
                    tr("Enter a name for the new relation (or press Cancel):"),
                    QLineEdit::Normal,QString::null, &ok );
    }
    if (ok && !newRelationName.isEmpty()){
        editRelationChangeCombo->addItem(newRelationName);
        emit addRelationToGraph(newRelationName);
        if (relationsCounter != 0){ //dont do it if its the first relation added
            qDebug() << "MW::slotEditRelationAdd() - updating combo index";
            editRelationChangeCombo->setCurrentIndex(relationsCounter);
        }
    }
    else if ( newRelationName.isEmpty() && ok ){
        QMessageBox::critical(this, tr("Error"),
                              tr("You did not type a name for this new relation"),
                              QMessageBox::Ok, 0);
        slotEditRelationAdd();
    }
    else {
        statusMessage( QString(tr("New relation cancelled.")) );
        return;
    }
    statusMessage( QString(tr("New relation named %1, added."))
                   .arg( newRelationName ) );
}







/**
 * @brief MainWindow::slotExportPNG
 * @return
 * Exports the network to a PNG image - Mediocre Quality but smaller file
 */
bool MainWindow::slotNetworkExportPNG(){
    qDebug()<< "MW::slotNetworkExportPNG";
    if (!fileLoaded && !networkModified )  {
        QMessageBox::critical(this, "Error",
                              tr("The canvas is empty!\n"
                              "Load a network file or create a new network first."), "OK",0);
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
    if (appSettings["printLogo"]=="true") {
        QImage logo(":/images/socnetv-logo.png");
        p.drawImage(5,5, logo);
        p.drawText(7,47,tempFileNameNoPath.last());
    }
    else
        p.drawText(5,15,tempFileNameNoPath.last());
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
 * @brief MainWindow::slotNetworkExportBMP
 * @return
 * Exports the network to a BMP image - Better Quality but larger file
 */
bool MainWindow::slotNetworkExportBMP(){
    qDebug(	"slotNetworkExportBMP()");
    if (!fileLoaded && !networkModified )  {
        QMessageBox::critical(this,
                              "Error",
                              tr(
                                  "Nothing to export! \n"
                                 "Load a network file or create a new network first."),
                              "OK",0);
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
    qDebug("slotNetworkExportBMP: grabbing canvas");
    picture=QPixmap::grabWidget(graphicsWidget, graphicsWidget->viewport()->rect());
    QPainter p;
    qDebug("slotNetworkExportBMP: adding logo");
    p.begin(&picture);
    p.setFont(QFont ("Helvetica", 10, QFont::Normal, false));
    if (appSettings["printLogo"]=="true") {
        QImage logo(":/images/socnetv-logo.png");
        p.drawImage(5,5, logo);
        p.drawText(7,47,tempFileNameNoPath.last());
    }
    else
        p.drawText(5,15,tempFileNameNoPath.last());
    p.end();
    qDebug("slotNetworkExportBMP: checking file");
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
 * @brief MainWindow::slotExportPDF
 * @return
 * Exports the network to a PDF Document - Best Quality
 */
bool MainWindow::slotNetworkExportPDF(){
    qDebug()<< "MW::slotNetworkExportPDF()";
    if (!fileLoaded && !networkModified )  {
        QMessageBox::critical(this,
                              "Error",
                              tr("The canvas is empty!\n"
                                 "Load a network file or create a new network first."), "OK",0);
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
        QPainter p;
        p.begin(&printer);
        graphicsWidget->render(&p);
        p.end();

    }
    qDebug()<< "Exporting PDF to "<< m_fileName;
    tempFileNameNoPath=m_fileName.split ("/");
    setLastPath(m_fileName);
    QMessageBox::information(this, tr("Export to PDF..."),
                             tr("File saved as: ")+tempFileNameNoPath.last() ,
                             "OK",0);
    statusMessage(  tr("Exporting completed") );
    return true;
}




/**
 * @brief MainWindow::slotExportPajek
 * Exports the network to a Pajek-formatted file
 * Calls the relevant Graph method.
 */
void MainWindow::slotNetworkExportPajek()
{
    qDebug () << "MW::slotNetworkExportPajek";

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



/**
 * @brief MainWindow::slotNetworkExportSM
 * Exports the network to a adjacency matrix-formatted file
 * Calls the relevant Graph method.
 */
void MainWindow::slotNetworkExportSM(){
    qDebug("MW: slotNetworkExportSM()");
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
 * @brief MainWindow::slotNetworkExportDL
 * @return Exports the network to a DL-formatted file
 * - TODO slotNetworkExportDL
 */
bool MainWindow::slotNetworkExportDL(){
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
    TODO slotNetworkExportGW
*/ 
bool MainWindow::slotNetworkExportGW(){
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
    TODO slotNetworkExportList
*/
bool MainWindow::slotNetworkExportList(){
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
 * @brief MainWindow::slotNetworkFileView
 * Displays the file of the loaded network.
   Network _must_ be unchanged since last save/load.
   Otherwise it will ask the user to first save the network, then view its file.
 */
void MainWindow::slotNetworkFileView(){
    qDebug() << "slotNetworkFileView() : " << fileName.toLatin1();
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
        slotNetworkSaveAs();
    }
    else if (fileLoaded && networkModified ) {   //file network + modified
        QMessageBox::information (this, "Viewing network file",
                                  //FIXME maybe better to save automagically rather than asking?
                                  tr("The network has been modified. \nI will save it to the original file for you now."), "OK",0);
        networkModified = false;
        slotNetworkSave();
        slotNetworkFileView();
    }
    else	{
        QMessageBox::critical(this, "Error",
                              tr("Empty network! \nLoad a network file first or create and save a new one..."), "OK",0);
        statusMessage(  tr("Nothing here. Not my fault, though!") );
    }
}




/**
 * @brief MainWindow::slotNetworkTextEditor
 * Opens the embedded text editor
 */
void MainWindow::slotNetworkTextEditor(){
    qDebug() << "slotNetworkTextEditor() : ";

    TextEditor *ed = new TextEditor("", this);
    ed->setWindowTitle(tr("New Network File"));
    ed->show();
    statusMessage(  tr("Enter your network data here" ) );
}





/**
 * @brief MainWindow::slotNetworkViewSociomatrix
 *  Displays the adjacency matrix of the network.
 *  It uses a different method for writing the matrix to a file.
 *  While slotNetworkExportSM uses << operator of Matrix class
 *  (via adjacencyMatrix of Graph class), this is using directly the
 *  writeAdjacencyMatrix method of Graph class
 */
void MainWindow::slotNetworkViewSociomatrix(){
    if ( !fileLoaded && !networkModified) {
        QMessageBox::critical (this, "Error",
                               tr("Empty network! \nLoad a network file or create something by double-clicking on the canvas!"), "OK",0);

        statusMessage(  tr("Nothing to show!") );
        return;
    }
    int aNodes=activeNodes();
    statusBar() ->  showMessage ( QString (tr ("creating adjacency adjacency matrix of %1 nodes")).arg(aNodes) );
    qDebug ("MW: calling Graph::writeAdjacencyMatrix with %i nodes", aNodes);
    QString fn = appSettings["dataDir"] + "socnetv-report-adjacency-matrix.dat";

    activeGraph.writeAdjacencyMatrix(fn, networkName.toLocal8Bit()) ;

    //Open a text editor window for the new file created by graph class
    TextEditor *ed = new TextEditor(fn);
    ed->show();
    statusMessage(tr("Adjacency Matrix saved as ") + fn);
}



/**
 * @brief MainWindow::slotNetworkDataSetSelect
 * Calls the m_datasetSelectionDialog to display the dataset selection dialog
 */
void MainWindow::slotNetworkDataSetSelect(){
    qDebug()<< "MW::slotNetworkDataSetSelect()";
    m_datasetSelectDialog.exec();
}



/**
 * @brief MainWindow::slotNetworkDataSetRecreate
 * @param m_fileName
 * Recreates some of the most famous and widely used data sets in
 * network analysis studies
 */
void MainWindow::slotNetworkDataSetRecreate (const QString m_fileName) {
    int m_fileFormat=0;
    qDebug()<< "MW::slotNetworkDataSetRecreate() fileName: " << m_fileName;

    //initNet();

    qDebug()<< "MW::slotNetworkDataSetRecreate() datadir+fileName: "
            << appSettings["dataDir"]+m_fileName;
    activeGraph.writeDataSetToFile(appSettings["dataDir"], m_fileName);

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
    if ( slotNetworkFileLoad(appSettings["dataDir"]+m_fileName, "UTF-8", m_fileFormat) ) {
        qDebug() << "slotNetworkDataSetRecreate() loaded file " << m_fileName;
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
 * @brief MainWindow::slotRandomErdosRenyiDialog
 * Shows the Erdos-Renyi network creation dialog
 */
void MainWindow::slotRandomErdosRenyiDialog(){

    statusMessage( "Creating a random symmetric network... ");

    m_randErdosRenyiDialog = new RandErdosRenyiDialog(this);

    connect( m_randErdosRenyiDialog, &RandErdosRenyiDialog::userChoices,
             this, &MainWindow::slotRandomErdosRenyi );

    m_randErdosRenyiDialog->exec();

}




/**
 * @brief MainWindow::slotRandomErdosRenyi
 * @param newNodes
 * @param model
 * @param edges
 * @param eprob
 * @param mode
 * @param diag
 * Calls activeGraph.slotRandomErdosRenyi () to create a symmetric network
 * Edge existance is controlled by a user specified possibility.
 */
void MainWindow::slotRandomErdosRenyi( const int newNodes,
                                       const QString model,
                                       const int edges,
                                       const float eprob,
                                       const QString mode,
                                       const bool diag)
{
    qDebug() << "MW::slotRandomErdosRenyi()";

    statusMessage( tr("Erasing any existing network."));
    initNet();

    statusMessage( tr("Creating random network. Please wait... ")  );


    QString msg = "Creating random network. \n "
                " Please wait (or disable progress bars from Options -> Settings).";
    createProgressBar(2*newNodes, msg);

    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

    activeGraph.createRandomNetErdos ( newNodes,
                                       model,
                                       edges,
                                       eprob,
                                       mode,
                                       diag);

    fileLoaded=false;

    slotEditNodeSizeAllNormalized(0);
    slotNetworkChanged();

    setWindowTitle("Untitled");

    double threshold = log(newNodes)/newNodes;
    float clucof=activeGraph.clusteringCoefficient();

    QApplication::restoreOverrideCursor();
    destroyProgressBar();

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
 * @brief MainWindow::slotRandomRegularNetwork
 * Creates a pseudo-random network where every node has the same degree
 */
void MainWindow::slotRandomRegularNetwork(){
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

    QString msg = "Creating random network. \n"
            "Please wait (or disable progress bars from Options -> Settings).";
    createProgressBar(2*newNodes, msg);


    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

    activeGraph.createSameDegreeRandomNetwork (newNodes,
                                               degree);

    QApplication::restoreOverrideCursor();

    destroyProgressBar();

    fileLoaded=false;

    slotNetworkChanged();
    setWindowTitle("Untitled");
    statusMessage( "Uniform random network created: "
                   +QString::number(activeNodes())+" Nodes, "
                   +QString::number( activeEdges())+" Edges");

}




void MainWindow::slotRandomGaussian(){
    slotNetworkChanged();

}



/**
 * @brief MainWindow::slotRandomScaleFreeDialog
 */
void MainWindow::slotRandomScaleFreeDialog() {
    qDebug() << "MW;:slotRandomScaleFreeDialog()";
    m_randScaleFreeDialog = new RandScaleFreeDialog(this);

    connect( m_randScaleFreeDialog, &RandScaleFreeDialog::userChoices,
             this, &MainWindow::slotRandomScaleFree);

    m_randScaleFreeDialog->exec();

}


/**
 * @brief MainWindow::slotRandomScaleFree
 * @param nodes
 * @param power
 * @param initialNodes
 * @param edgesPerStep
 * @param zeroAppeal
 * @param mode
 */
void MainWindow::slotRandomScaleFree ( const int &nodes,
                                          const int &power,
                                          const int &initialNodes,
                                          const int &edgesPerStep,
                                          const float &zeroAppeal,
                                          const QString &mode)
{
    qDebug() << "MW;:slotRandomScaleFree()";
    statusMessage( tr("Erasing any existing network. "));
    initNet();
    statusMessage( tr("Creating small world network. Please wait..."));

    double x0=scene->width()/2.0;
    double y0=scene->height()/2.0;
    double radius=(graphicsWidget->height()/2.0)-50;          //pixels

    if (appSettings["showProgressBar"] == "true" && nodes > 100){
        progressDialog= new QProgressDialog(
                    tr("Creating random network. Please wait \n (or disable progress bars from Options -> Settings)."),
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

    if (appSettings["showProgressBar"] == "true" && nodes > 100 )
        progressDialog->deleteLater();

    fileLoaded=false;

    slotNetworkChanged();
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



/**
 * @brief MainWindow::slotRandomSmallWorldDialog
 */
void MainWindow::slotRandomSmallWorldDialog()
{
    qDebug() << "MW::slotRandomSmallWorldDialog()";
    m_randSmallWorldDialog = new RandSmallWorldDialog(this);

    connect( m_randSmallWorldDialog, &RandSmallWorldDialog::userChoices,
             this, &MainWindow::slotRandomSmallWorld);


    m_randSmallWorldDialog->exec();

}


/**
 * @brief MainWindow::slotrandomSmallWorldNetwork
 * @param nodes
 * @param degree
 * @param beta
 * @param mode
 * @param diag
 */
void MainWindow::slotRandomSmallWorld(const int &nodes,
                                            const int &degree,
                                            const float &beta,
                                            const QString &mode,
                                            const bool &diag)
{
    Q_UNUSED(diag);
    qDebug() << "MW::slotRandomSmallWorld()";
    statusMessage( tr("Erasing any existing network. "));
    initNet();
    statusMessage( tr("Creating small world network. Please wait..."));

    double x0=scene->width()/2.0;
    double y0=scene->height()/2.0;
    double radius=(graphicsWidget->height()/2.0)-50;          //pixels

    if (appSettings["showProgressBar"] == "true" && nodes > 100){
        progressDialog= new QProgressDialog(
                    tr("Creating random network. Please wait \n (or disable progress bars from Options -> Settings )."),
                    "Cancel", 0, (int) (2* nodes), this);
        progressDialog -> setWindowModality(Qt::WindowModal);
        connect( &activeGraph, SIGNAL( updateProgressDialog(int) ), progressDialog, SLOT(setValue(int) ) ) ;
        progressDialog->setMinimumDuration(0);
    }
    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
    activeGraph.createRandomNetSmallWorld(nodes, degree, beta, x0, y0, radius);
    activeGraph.symmetrize();
    QApplication::restoreOverrideCursor();

    if (appSettings["showProgressBar"] == "true" && nodes > 100 )
        progressDialog->deleteLater();

    fileLoaded=false;

    slotNetworkChanged();
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
 * @brief MainWindow::slotRandomRingLattice
 * Creates a lattice network, i.e. a connected network where every node
    has the same degree and is Edgeed with its neighborhood.
 */
void MainWindow::slotRandomRingLattice(){
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

    if (appSettings["showProgressBar"] == "true" && newNodes > 100){
        progressDialog= new QProgressDialog("Creating random network. Please wait (or disable progress bars from Options -> Settings).", "Cancel", 0, (int) (newNodes+newNodes), this);
        progressDialog -> setWindowModality(Qt::WindowModal);
        connect( &activeGraph, SIGNAL( updateProgressDialog(int) ), progressDialog, SLOT(setValue(int) ) ) ;
        progressDialog->setMinimumDuration(0);
    }

    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

    activeGraph.createRandomNetRingLattice(newNodes, degree, x0, y0, radius );

    QApplication::restoreOverrideCursor();

    if (appSettings["showProgressBar"] == "true" && newNodes > 100)
        progressDialog->deleteLater();

    fileLoaded=false;

    //	slotNetworkChanged();

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
 * @brief MainWindow::slotNetworkWebCrawlerDialog
 * Shows a dialog where enters a website url
 * and the app creates a new network by crawling it
 */
void MainWindow::slotNetworkWebCrawlerDialog() {
    qDebug () << "MW: slotNetworkWebCrawlerDialog() - canvas Width & Height already sent";
    m_WebCrawlerDialog.exec() ;
}






/**
 * @brief MainWindow::slotNetworkWebCrawler
 * Called from m_WebCrawlerDialog
 * Clears the loaded network (saving if needed) then passes parameters to
 * Graph::webCrawl function
 * @param seed
 * @param maxNodes
 * @param maxRecursion
 * @param extLinks
 * @param intLinks
 */
void MainWindow::slotNetworkWebCrawler ( QString  seed, int maxNodes, int maxRecursion,
                                bool extLinks, bool intLinks) {
    this->slotNetworkClose();
    activeGraph.webCrawl( seed, maxNodes, maxRecursion,  extLinks, intLinks) ;

}





/**
 * @brief MainWindow::slotNetworkChanged
 * Activated when something has been changed in the graph.
 * Makes the networkSave icon active and refreshes any LCD values.
 * Also called from activeGraph and graphicsWidget.
 */
void MainWindow::slotNetworkChanged(){
    qDebug("MW: slotNetworkChanged");
    networkModified=true;
    networkSave->setIcon(QIcon(":/images/save.png"));
    networkSave->setEnabled(true);

    nodesLCD->display(activeGraph.vertices());
    edgesLCD->display(activeEdges());
    densityLCD->display( activeGraph.density() );
}








/**
 * @brief MainWindow::slotEditOpenContextMenu
 * Popups a context menu with some options when the user right-clicks on the scene
 * @param mPos
 */
void MainWindow::slotEditOpenContextMenu( const QPointF &mPos) {
    cursorPosGW=mPos;
    QMenu *contextMenu = new QMenu(" Menu",this);
    Q_CHECK_PTR( contextMenu );  //displays "out of memory" if needed



    contextMenu -> addAction( "## Selected nodes: "
                              + QString::number(  selectedNodes().count() ) + " ##  ");

    contextMenu -> addSeparator();

    if (selectedNodes().count()) {
        contextMenu -> addAction(editNodePropertiesAct );
        contextMenu -> addSeparator();
    }

    contextMenu -> addAction( editNodeAddAct );
    contextMenu -> addAction( editEdgeAddAct );
    contextMenu -> addSeparator();

    QMenu *options=new QMenu("Options", this);
    contextMenu -> addMenu(options );

    options -> addAction (openSettingsAct  );
    options -> addSeparator();
    options -> addAction (editNodeSizeAllAct );
    options -> addAction (editNodeShapeAll  );
    options -> addAction (editNodeColorAll );
    options -> addAction (optionsNodeNumbersVisibilityAct);
    options -> addAction (optionsNodeLabelsVisibilityAct);
    options -> addSeparator();
    options -> addAction (changeAllEdgesColorAct  );
    options -> addSeparator();
    options -> addAction (changeBackColorAct  );
    options -> addAction (backgroundImageAct  );

    //QCursor::pos() is good only for menus not related with node coordinates
    contextMenu -> exec(QCursor::pos() );
    delete  contextMenu;
    cursorPosGW=QPoint(-1,-1);
}



/**
 * @brief MainWindow::selectedNodes
 * Returns a QList of all selected nodes
 * @return
 */
QList<QGraphicsItem *> MainWindow::selectedNodes() {
    return graphicsWidget->selectedItems();

}


/**
 * @brief MainWindow::slotEditClickOnEmptySpace
 * Called from GW when the user clicks on empty space.
 */
void MainWindow::slotEditClickOnEmptySpace() {
    selectedNodeLCD->display (0);
    inDegreeLCD->display (0);
    outDegreeLCD->display (0);
    clucofLCD->display(0);
}



/**
 * @brief MainWindow::slotEditNodeSelectAll
 */
void MainWindow::slotEditNodeSelectAll(){
    qDebug() << "MainWindow::slotEditNodeSelectAll()";
    graphicsWidget->selectAll();
    statusMessage( QString(tr("Selected nodes: %1") )
                   .arg( selectedNodes().count() ) );

}


/**
 * @brief MainWindow::slotEditNodeSelectNone
 */
void MainWindow::slotEditNodeSelectNone(){
    qDebug() << "MainWindow::slotEditNodeSelectNone()";
    graphicsWidget->selectNone();
    statusMessage( QString(tr("Selection cleared") ) );
}


/**
 * @brief MainWindow::slotEditNodeAdd
 * Calls Graph::createVertex method to add a new RANDOM node into the activeGraph.
 * Called when "Add Node" button is clicked on the Main Window.
 */
void MainWindow::slotEditNodeAdd() {
    qDebug() << "MW::slotEditNodeAdd() ";
    // minus a  screen edge offset...
    activeGraph.createVertex (
                -1, graphicsWidget->width()-10,  graphicsWidget->height()-10);
    statusMessage( tr("New node (numbered %1) added.")
                   .arg(activeGraph.lastVertexNumber())  );
}



/**
 * @brief MainWindow::slotEditNodeAddWithMouse
 * Called by GW when user double-clicks to add a new node
 * Calls Graph::createVertex method to add a new node into the activeGraph.
 * @param num
 * @param p
 */
void MainWindow::slotEditNodeAddWithMouse(int num, QPointF p) {
    qDebug("MW: slotEditNodeAddWithMouse(). Calling activeGraph::createVertex() for a vertex named %i", num);
    activeGraph.createVertex(num, p);
    statusMessage( tr("New node (numbered %1) added.").arg(activeGraph.lastVertexNumber())  );
}



/**
 * @brief MainWindow::slotEditNodeFind
 * Calls GW::setMarkedNode() to find a node by its number or label.
 * The node is then marked.
 */
void MainWindow::slotEditNodeFind(){
    qDebug ("MW: slotEditNodeFind()");
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
 * @brief MainWindow::slotEditNodeRemove
 * Deletes a node and the attached objects (edges, etc).
 * It deletes clickedJim (signal from GraphicsView or set by another function)
 * or else asks for a nodeNumber to remove. The nodeNumber is doomedJim.
 * Called from nodeContextMenu
 */
void MainWindow::slotEditNodeRemove() {
    qDebug() << "MW: slotEditNodeRemove()";
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
    slotNetworkChanged();
    qDebug("MW: removeNode() completed. Node %i removed completely.",doomedJim);
    statusMessage( tr("Node removed completely. Ready. ") );
}



/**
 * @brief MainWindow::slotEditNodePropertiesDialog
 * Reads values from selected nodes
 * then open Node Properties dialog
 */
void MainWindow::slotEditNodePropertiesDialog() {

    qDebug() << "MW::slotEditNodePropertiesDialog()";
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
    int min=-1, max=-1, size = appSettings["initNodeSize"].toInt(0, 10);
    QColor color = QColor(appSettings["initNodeColor"]);
    QString shape= appSettings["initNodeShape"];
    QString label="";
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
             this, &MainWindow::slotEditNodeProperties );

    m_nodeEditDialog->exec();

    statusMessage( tr("Node properties dialog opened. Ready. ") );
}


/**
 * @brief MainWindow::slotEditNodeProperties
 * Applies new (user-defined) values to all selected nodes
 * Called on exit from NodeEditDialog
 * @param label
 * @param size
 * @param value
 * @param color
 * @param shape
 */
void MainWindow::slotEditNodeProperties( const QString label, const int size,
                                     const QString value, const QColor color,
                                     const QString shape) {
    qDebug()<< "MW::slotEditNodeProperties() "
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

            if (appSettings["initNodeLabelsVisibility"] != "true")
                slotOptionsNodeLabelsVisibility(true);

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

    slotNetworkChanged();
    statusMessage( tr("Ready. "));

}






/**
 * @brief MainWindow::slotEditNodeColorAll
 * Opens a QColorDialog to select a new node color for all nodes.
 * Calls  activeGraph.setAllVerticesColor to do the work
 * Called from Edit menu option
 */
void MainWindow::slotEditNodeColorAll(){
    QColor color = QColorDialog::getColor( Qt::red, this,
                                           "Change the color of all nodes" );
    if (color.isValid()) {
        appSettings["initNodeColor"] = color.name();
        QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
        qDebug() << "MW::slotEditNodeColorAll() : " << appSettings["initNodeColor"];
        activeGraph.setAllVerticesColor(appSettings["initNodeColor"]);
        QApplication::restoreOverrideCursor();
        statusMessage( tr("Ready. ")  );
    }
    else {
        // user pressed Cancel
        statusMessage( tr("Nodes color change aborted. ") );
    }
}




/**
 * @brief MainWindow::slotEditNodeColorAll
 * Overloaded
 * Changes the color of all nodes to parameter color
 * Calls  activeGraph.setAllVerticesColor to do the work
 * Called from Settings Dialog
 * @param color
 */
void MainWindow::slotEditNodeColorAll(QColor color){
    if (color.isValid()) {
        appSettings["initNodeColor"] = color.name();
        QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
        qDebug() << "MW::slotEditNodeColorAll() : " << appSettings["initNodeColor"];
        activeGraph.setAllVerticesColor(appSettings["initNodeColor"]);
        QApplication::restoreOverrideCursor();
        statusMessage( tr("Ready. ")  );
    }
    else {
        // user pressed Cancel
        statusMessage( tr("Invalid color. ") );
    }
}



/**
 * @brief MainWindow::slotEditNodeSizeAll
 * Asks the user a new size for all nodes
 * Calls MW::slotEditNodeSizeAllNormalized to do the work.
 * Called when user clicks on relevant Edit menu option
 */
void MainWindow::slotEditNodeSizeAll() {
    qDebug ("MW: slotEditNodeSizeAll:");
    bool ok=false;
    int newSize = QInputDialog::getInt(
                this,
                "Change node size",
                tr("Select new size for all nodes: (1-16)"),
                appSettings["initNodeSize"].toInt(0, 10), 1, 16, 1, &ok );
    if (!ok) {
        statusMessage( "Change node size operation cancelled." );
        return;
    }
    appSettings["initNodeSize"]= QString::number(newSize);
    slotEditNodeSizeAllNormalized(newSize);
    slotNetworkChanged();
    statusBar()->showMessage (QString(tr("Ready")), statusBarDuration) ;
    return;
}



/**
 * @brief MainWindow::slotEditNodeSizeAllNormalized
 * Changes the size of nodes to size.
 * Calls activeGraph.setAllVerticesSize to do the work.
 * Called from MainWindow::slotEditNodeSizeAll and MW::slotRandomErdosRenyi
 * as well as from SettingsDialog
 * @param size
 */
void MainWindow::slotEditNodeSizeAllNormalized(int size) {
    qDebug() << "MW: slotEditNodeSizeAllNormalized:" << size;
    if (size == 0 ) {
        if (activeNodes() < 200) {
            return;
        }
        else if (activeNodes() >= 200 && activeNodes() < 500){
            size = 6;
        }
        else if (activeNodes() >= 500 && activeNodes() < 1000) {
            size = 6;
        }
        else if (activeNodes() >= 1000) {
            size = 5;
        }
    }
    activeGraph.setAllVerticesSize(size);
}



/**
 * @brief MainWindow::slotEditNodeShapeAll
 * Called when user clicks on Edit -> Node > Change all nodes shapes
 * Offers the user a list of available node shapes to select.
 * Then changes the shape of all nodes/vertices accordingly.
 */
void MainWindow::slotEditNodeShapeAll() {
    bool ok=false;
    QStringList lst;
    lst << "box"<< "circle"<< "diamond"<< "ellipse"<< "triangle";
    QString newShape = QInputDialog::getItem(this, "Node shapes", "Select a shape for all nodes: ", lst, 1, true, &ok);
    if ( ok ) {
        //user selected an item and pressed OK
//        QList<QGraphicsItem *> list=scene->items();
//        for (QList<QGraphicsItem *>::iterator it=list.begin(); it!=list.end(); it++)
//            if ( (*it) -> type() == TypeNode ){
//                Node *jim = (Node*) (*it);
//                (*jim).setShape(newShape);
//                activeGraph.setVertexShape ((*jim).nodeNumber(), newShape);
//            }
        slotNetworkChanged();
        appSettings["initNodeShape"] = newShape;
        activeGraph.setAllVerticesShape(newShape);
        statusBar()->showMessage (QString(tr("All shapes have been changed. Ready")), statusBarDuration) ;
    } else {
        //user pressed Cancel
        statusBar()->showMessage (QString(tr("Change node shapes aborted...")), statusBarDuration) ;
    }

}


/**
 * @brief MainWindow::slotEditNodeShape
 * Called from SettingsDialog when the user has selected a new default node shape
 * Calls Graph::setAllVerticesShape(QString)
 * @param shape
 *
 */
void MainWindow::slotEditNodeShape(const QString shape, const int vertex) {

    qDebug() << "MW::slotEditNodeShape() - vertex " << vertex
             << " (0 means all) - new shape " << shape;

    if (vertex == 0) { //change all nodes shapes
//        QList<QGraphicsItem *> list=scene->items();
//        for (QList<QGraphicsItem *>::iterator it=list.begin(); it!=list.end(); it++)
//            if ( (*it) -> type() == TypeNode ){
//                Node *jim = (Node*) (*it);
//                (*jim).setShape(shape);
//                activeGraph.setVertexShape ((*jim).nodeNumber(), shape);
//            }
        slotNetworkChanged();
        activeGraph.setAllVerticesShape(shape);
        statusBar()->showMessage (QString(tr("All shapes have been changed. Ready")), statusBarDuration) ;

    }
    else { //only one

       activeGraph.setVertexShape( vertex, shape);
//       QList<QGraphicsItem *> list=scene->items();
//       for (QList<QGraphicsItem *>::iterator it=list.begin(); it!=list.end(); it++)
//           if ( (*it) -> type() == TypeNode ){
//               Node *jim = (Node*) (*it);
//               if ( (*jim).nodeNumber() ==  vertex ) {
//                 (*jim).setShape(shape);
//               }
//           }
      }
}




/**
 * @brief MainWindow::slotEditNodeNumbersSize
 * Changes the size of all nodes' numbers.
 * Asks the user to enter a new node number font size
 * Called from Edit menu option
 */
void MainWindow::slotEditNodeNumbersSize() {
    bool ok=false;
    int newSize;
    newSize = QInputDialog::getInt(this, "Change text size", tr("Change all nodenumbers size to: (1-16)"),appSettings["initNumberSize"].toInt(0,10), 1, 16, 1, &ok );
    if (!ok) {
        statusMessage( tr("Change font size: Aborted.") );
        return;
    }

    QList<QGraphicsItem *> list=scene->items();
    for (QList<QGraphicsItem *>::iterator it2=list.begin();it2!=list.end(); it2++)

        if ( (*it2)->type()==TypeNumber) {
            NodeNumber * number= (NodeNumber*) (*it2);
            qDebug ("MW: slotEditNodeNumbersSize Found");
            number->setFont( QFont (number->font().family(), newSize, QFont::Light, false) );
        }

    activeGraph.setInitVertexNumberSize(newSize);
    statusMessage( tr("Changed numbers size. Ready.") );
}


/**
 * @brief MainWindow::slotEditNodeLabelsSize
 * Changes the size of all nodes' labels.
 * Asks the user to enter a new node label font size
 */
void MainWindow::slotEditNodeLabelsSize() {
    bool ok=false;
    int newSize;
    newSize = QInputDialog::getInt(this, "Change text size", tr("Change all node labels size to: (1-16)"),appSettings["initLabelSize"].toInt(0,10), 1, 16, 1, &ok );
    if (!ok) {
        statusMessage( tr("Change font size: Aborted.") );
        return;
    }
    QList<QGraphicsItem *> list=scene->items();
    for (QList<QGraphicsItem *>::iterator it2=list.begin();it2!=list.end(); it2++)

        if ( (*it2)->type()==TypeLabel) {
            NodeLabel *label= (NodeLabel*) (*it2);
            qDebug ("MW: slotEditNodeLabelsSize Found");
            label->setFont( QFont (label->font().family(), newSize, QFont::Light, false) );
            activeGraph.setVertexLabelSize ( (label->node())->nodeNumber(), newSize);
        }
    activeGraph.setInitVertexLabelSize(newSize);
    statusMessage( tr("Changed labels size. Ready.") );
}





/**
 * @brief MainWindow::slotEditNodeNumbersColor
 * Changes the color of all nodes' numbers.
 * Asks the user to enter a new node number color
 */
void MainWindow::slotEditNodeNumbersColor(){

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
 * @brief MainWindow::slotEditNodeLabelsColor
 * Changes the color of all nodes' labels.
 * Asks the user to enter a new node label color
 */
void MainWindow::slotEditNodeLabelsColor(){
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
 * @brief MainWindow::slotEditNodeOpenContextMenu
 * Called from GW when the user has right-clicked on a node
 * Opens a node context menu with some options when the user right-clicks on a node
 */
void MainWindow::slotEditNodeOpenContextMenu() {
    clickedJimNumber=clickedJim->nodeNumber();
    qDebug("MW: slotEditNodeOpenContextMenu() for node %i at %i, %i",
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
    nodeContextMenu -> addAction(editEdgeAddAct);
    nodeContextMenu -> addAction(editNodeRemoveAct );
    nodeContextMenu -> addAction(editNodePropertiesAct );
    //QCursor::pos() is good only for menus not related with node coordinates
    nodeContextMenu -> exec(QCursor::pos() );
    delete  nodeContextMenu;
    clickedJimNumber=-1;    //undo node selection
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
    edgeContextMenu -> addAction( editEdgeRemoveAct );
    edgeContextMenu -> addAction( changeEdgeWeightAct );
    edgeContextMenu -> addAction( changeEdgeColorAct );
    edgeContextMenu -> exec(QCursor::pos() );
    delete  edgeContextMenu;
}


/**
 * @brief MainWindow::slotEditEdgeAdd
 * Adds a new edge between two nodes specified by the user.
 * Called when user clicks on the MW button/menu item "Add edge"
 */
void MainWindow::slotEditEdgeAdd(){
    qDebug ("MW: slotEditEdgeAdd()");
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
        qDebug ("MW: slotEditEdgeAdd: Cant find sourceNode %i.", sourceNode);
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
        qDebug ("MW: slotEditEdgeAdd: Cant find targetNode %i",targetNode);
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

    slotEditEdgeCreate(sourceNode, targetNode, weight);
    slotNetworkChanged();
    statusMessage( tr("Ready. ")  );
}



/**
 * @brief MainWindow::slotEditEdgeCreate
 * helper to slotEditEdgeAdd() above
 * Also called from GW::userMiddleClicked() signal when user creates edges with middle-clicks
 * Calls Graph::createEdge method to add the new edge to the active Graph
  * @param v1
 * @param v2
 * @param weight
 */
void MainWindow::slotEditEdgeCreate (int v1, int v2, float weight) {
    qDebug("MW: slotEditEdgeCreate() - setting user settings and calling Graph::createEdge(...)");
    bool drawArrows=displayEdgesArrowsAct->isChecked();
    int reciprocal=0;
    bool bezier = false;
    activeGraph.createEdge(v1, v2, weight, reciprocal, drawArrows, bezier);

    if ( activeEdges() == 1 && editRelationChangeCombo->count() == 0 ) {
        slotEditRelationAdd();
    }
}



/**
 * @brief MainWindow::slotEditEdgeRemove
 * Erases the clicked edge. Otherwise asks the user to specify one edge.
 * First deletes arc reference from object nodeVector then deletes arc item from scene
 */
void MainWindow::slotEditEdgeRemove(){
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
                graphicsWidget->drawEdge(targetNode, sourceNode, 1, false, displayEdgesArrowsAct->isChecked(), appSettings["initEdgeColor"], false);

                break;
            case 1:
                clickedEdge->unmakeReciprocal();
                //graphicsWidget->removeItem(clickedEdge);
                activeGraph.removeEdge(targetNode, sourceNode);
                //						graphicsWidget->drawEdge(i, j, false, drawArrowsAct->isChecked(), appSettings["initEdgeColor"], false);
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
    slotNetworkChanged();
    qDebug("MW: View items now: %i ", graphicsWidget->items().size());
    qDebug("MW: Scene items now: %i ", scene->items().size());
}








//TODO slotChangeEdgeLabel
void MainWindow::slotChangeEdgeLabel(){
    slotNetworkChanged();
}





/**
*  Changes the color of all edges
*/
void MainWindow::slotAllEdgesColor(){
    QColor color = QColorDialog::getColor( Qt::red, this,
                                           "Change the color of all nodes" );
    if (color.isValid()) {
        appSettings["initEdgeColor"]=color.name();
        QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
        qDebug() << "MainWindow::slotAllEdgesColor() : " << appSettings["initEdgeColor"];
        //createProgressBar();
        activeGraph.setAllEdgesColor(appSettings["initEdgeColor"]);
        //destroyProgressBar();
        QApplication::restoreOverrideCursor();
        slotNetworkChanged();
        statusMessage( tr("Ready. ")  );
    }
    else {
        // user pressed Cancel
        statusMessage( tr("edges color change aborted. ") );
    }
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

    int min=activeGraph.firstVertexNumber();
    int max=activeGraph.lastVertexNumber();

    if (!edgeClicked)
    {	//no edge clicked. Ask user to define an edge.
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

    }
    else
    {	//edge has been clicked.
         sourceNode = clickedEdge->sourceNodeNumber();
         targetNode = clickedEdge->targetNodeNumber();
    }

    QColor color = QColorDialog::getColor(
                Qt::black, this, tr("Select new color....") );

    if ( color.isValid()) {
        QString newColor=color.name();
        qDebug() << "MW::slotChangeEdgeColor() - " << sourceNode << " -> "
                    << targetNode << " newColor "
                 << newColor;
        activeGraph.setEdgeColor( sourceNode, targetNode, newColor);
        statusMessage( tr("Ready. ")  );
    }
    else {
        statusMessage( tr("Change edge color aborted. ") );
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
 *Calls Graph::filterIsolateVertices to toggle visibility of isolated vertices
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
    statusMessage(  QString(tr("Isolate nodes visibility toggled!"))  );
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
    slotNetworkChanged();

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
    Called from menu or toolbox checkbox
    Calls Graph::layoutForceDirectedSpringEmbedder to embed a
    spring-gravitational model
*/
void MainWindow::slotLayoutSpringEmbedder(){
    qDebug()<< "MW:slotLayoutSpringEmbedder";
    if (!fileLoaded && !networkModified  )  {
        QMessageBox::critical(this, "Error",tr("There are node nodes yet!\nLoad a network file or create a new network first. \nThen we can talk about layouts!"), "OK",0);
        statusMessage( tr("I am really sorry. You must really load a file first... ")  );

        return;
    }

    statusMessage( tr("Embedding a spring-gravitational model on the network.... ")  );
    //scene->setItemIndexMethod (QGraphicsScene::NoIndex); //best when moving items
    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
    createProgressBar();
    activeGraph.layoutForceDirectedSpringEmbedder(100);
    destroyProgressBar();
    QApplication::restoreOverrideCursor();
    //scene->setItemIndexMethod (QGraphicsScene::BspTreeIndex); //best when not moving items
}





/**
    Called from menu or toolbox
    Calls Graph::layoutForceDirectedFruchtermanReingold to embed
    a repelling-attracting forces model
*/
void MainWindow::slotLayoutFruchterman(){
    qDebug("MW: slotLayoutFruchterman ()");
    if (!fileLoaded && !networkModified  )  {
        QMessageBox::critical(this, "Error",tr("There are no nodes yet!\nLoad a network file or create a new network first. \nThen we can talk about layouts!"), "OK",0);
        statusMessage( tr("I am really sorry. You must really load a file first... ")  );
        return;
    }

    statusMessage( tr("Embedding a repelling-attracting forces model on the network.... ")  );
    //scene->setItemIndexMethod (QGraphicsScene::NoIndex); //best when moving items
    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
    createProgressBar();
    activeGraph.layoutForceDirectedFruchtermanReingold(100);

    destroyProgressBar();
    QApplication::restoreOverrideCursor();
    //scene->setItemIndexMethod (QGraphicsScene::BspTreeIndex); //best when not moving items

}




/**
 * @brief
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
 * @brief
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
 * @brief
 * Enables/disables layout guides
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
 * @brief
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
 * @brief
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
 * @brief
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
 * @brief
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
*	Returns the amount of enabled/active edges on the scene.
*/
int MainWindow::activeEdges(){
    qDebug () << "MW::activeEdges()";
    return activeGraph.enabledEdges();
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
    QString fn = appSettings["dataDir"] + "socnetv-report-invert-adjacency-matrix.dat";

    QTime timer;
    timer.start();
    activeGraph.writeInvertAdjacencyMatrix(fn, networkName, QString("lu")) ;
    int msecs = timer.elapsed();
    statusMessage (QString(tr("Ready.")) + QString(" Time: ") + QString::number(msecs) );
    //Open a text editor window for the new file created by graph class
    TextEditor *ed = new TextEditor(fn);
    ed->setWindowTitle(tr("Inverse adjacency matrix saved as ") + fn);
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
    QString fn = appSettings["dataDir"] + "socnetv-report-distance-matrix.dat";


    askAboutWeights();

    createProgressBar();
    activeGraph.writeDistanceMatrix(fn, networkName.toLocal8Bit(),
                                    considerWeights, inverseWeights,
                                    filterIsolateNodesAct->isChecked());

    destroyProgressBar();

    //Open a text editor window for the new file created by graph class
    TextEditor *ed = new TextEditor(fn);
    ed->show();
    QApplication::restoreOverrideCursor();
    statusMessage(tr("Distance matrix saved as: ")+fn);
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

    QString fn = appSettings["dataDir"] + "socnetv-report-sigmas-matrix.dat";

    askAboutWeights();

    statusMessage( tr("Creating number of geodesics matrix. Please wait...") );
    createProgressBar();

    activeGraph.writeNumberOfGeodesicsMatrix(fn, networkName.toLocal8Bit(),
                                             considerWeights, inverseWeights);

    destroyProgressBar();

    //Open a text editor window for the new file created by graph class

    TextEditor *ed = new TextEditor(fn);
    ed->show();
    QApplication::restoreOverrideCursor();
    statusMessage(tr("Matrix of geodesic path counts saved as: ") + fn);
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
                considerWeights, inverseWeights,  filterIsolateNodesAct->isChecked() );

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
    QString fn = appSettings["dataDir"] + "socnetv-report-eccentricity.dat";

    askAboutWeights();
    statusMessage(  QString(tr(" Please wait...")));

    createProgressBar();
    activeGraph.writeEccentricity(
                fn, considerWeights, inverseWeights,
                filterIsolateNodesAct->isChecked());
    destroyProgressBar();

    TextEditor *ed = new TextEditor(fn);        //OPEN A TEXT EDITOR WINDOW
    ed->show();
    QApplication::restoreOverrideCursor();
    statusMessage(tr("Eccentricity report saved as: ") + fn );
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

    QString fn = appSettings["dataDir"] + "socnetv-report-number-of-walks.dat";
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
    ed->show();
    QApplication::restoreOverrideCursor();
    statusMessage(tr("Number of walks saved as: ") + fn );
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
    QString fn = appSettings["dataDir"] + "socnetv-report-total-number-of-walks.dat";
    createProgressBar();

    int maxLength=activeNodes()-1;
    activeGraph.writeTotalNumberOfWalksMatrix(fn, networkName, maxLength);

    destroyProgressBar();

    TextEditor *ed = new TextEditor(fn);        //OPEN A TEXT EDITOR WINDOW
    ed->show();
    QApplication::restoreOverrideCursor();
    statusMessage("Total number of walks saved as: " + fn);

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

    QString fn = appSettings["dataDir"] + "socnetv-report-reachability-matrix.dat";

    createProgressBar();

    activeGraph.writeReachabilityMatrix(fn, networkName);

    destroyProgressBar();

    TextEditor *ed = new TextEditor(fn);        //OPEN A TEXT EDITOR WINDOW
    ed->show();
    QApplication::restoreOverrideCursor();
    statusMessage("Reachability Matrix saved as: " + fn );
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
    QString fn = appSettings["dataDir"] + "socnetv-report-clique-census.dat";
    bool considerWeights=true;

    createProgressBar();

    activeGraph.writeCliqueCensus(fn, considerWeights);

    destroyProgressBar();

    TextEditor *ed = new TextEditor(fn);        //OPEN A TEXT EDITOR WINDOW
    ed->show();
    QApplication::restoreOverrideCursor();
    statusMessage("Clique Census saved as: " + fn);
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
    QString fn = appSettings["dataDir"] + "socnetv-report-clustering-coefficients.dat";
    bool considerWeights=true;

    createProgressBar();

    activeGraph.writeClusteringCoefficient(fn, considerWeights);

    destroyProgressBar();

    TextEditor *ed = new TextEditor(fn);        //OPEN A TEXT EDITOR WINDOW
    ed->show();
    QApplication::restoreOverrideCursor();
    statusMessage("Clustering Coefficients saved as: " + fn);
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
    QString fn = appSettings["dataDir"] + "socnetv-report-triad-census.dat";
    bool considerWeights=true;

    createProgressBar();

    activeGraph.writeTriadCensus(fn, considerWeights);

    destroyProgressBar();

    TextEditor *ed = new TextEditor(fn);        //OPEN A TEXT EDITOR WINDOW
    ed->show();
    QApplication::restoreOverrideCursor();
    statusMessage("Triad Census saved as: " + fn);
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
    QString fn = appSettings["dataDir"] + "socnetv-report-centrality-out-degree.dat";

    createProgressBar();

    activeGraph.writeCentralityDegree(fn, considerWeights,
                                      filterIsolateNodesAct->isChecked() );

    destroyProgressBar();

    TextEditor *ed = new TextEditor(fn);        //OPEN A TEXT EDITOR WINDOW
    ed->show();
    QApplication::restoreOverrideCursor();
    statusMessage(tr("Out-Degree Centralities saved as: ") + fn);
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

    QString fn = appSettings["dataDir"] + "socnetv-report-centrality_closeness.dat";

    createProgressBar();

    activeGraph.writeCentralityCloseness(
                fn, considerWeights, inverseWeights,
                filterIsolateNodesAct->isChecked() || dropIsolates);

    destroyProgressBar();

    TextEditor *ed = new TextEditor(fn);        //OPEN A TEXT EDITOR WINDOW
    ed->show();
    QApplication::restoreOverrideCursor();
    statusMessage(tr("Closeness Centralities  saved as: ") + fn);
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

    QString fn = appSettings["dataDir"] + "socnetv-report-centrality_closeness_influence_range.dat";


    askAboutWeights();

    createProgressBar();

    activeGraph.writeCentralityClosenessInfluenceRange(
                fn, considerWeights,inverseWeights,
                filterIsolateNodesAct->isChecked());

    destroyProgressBar();

    statusMessage( QString(tr(" displaying file...")));

    TextEditor *ed = new TextEditor(fn);        //OPEN A TEXT EDITOR WINDOW
    ed->show();
    QApplication::restoreOverrideCursor();
    statusMessage(tr("Influence Range Closeness Centrality saved as: ")+fn);
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
    QString fn = appSettings["dataDir"] + "socnetv-report-centrality_betweenness.dat";


    askAboutWeights();

    statusMessage(  QString(tr(" Please wait...")));
    createProgressBar();
    activeGraph.writeCentralityBetweenness(
                fn, considerWeights, inverseWeights,
                filterIsolateNodesAct->isChecked());
    destroyProgressBar();

    statusMessage( QString(tr(" displaying file...")));

    TextEditor *ed = new TextEditor(fn);        //OPEN A TEXT EDITOR WINDOW
    ed->show();
    QApplication::restoreOverrideCursor();
    statusMessage(tr("Betweenness Centralities saved as: ")+fn);
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
    QString fn = appSettings["dataDir"] + "socnetv-report-degree-prestige.dat";

    createProgressBar();

    activeGraph.writePrestigeDegree(fn, considerWeights,
                                    filterIsolateNodesAct->isChecked() );

    destroyProgressBar();

    TextEditor *ed = new TextEditor(fn);        //OPEN A TEXT EDITOR WINDOW
    ed->show();
    QApplication::restoreOverrideCursor();
    statusMessage(tr("Degree Prestige (in-degree) saved as: ") + fn);
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
    QString fn = appSettings["dataDir"] + "socnetv-report-prestige_pagerank.dat";


    askAboutWeights();

    statusMessage(  QString(tr(" Please wait...")));

    createProgressBar();
    activeGraph.writePrestigePageRank(fn, filterIsolateNodesAct->isChecked());
    destroyProgressBar();

    TextEditor *ed = new TextEditor(fn);        //OPEN A TEXT EDITOR WINDOW
    ed->show();
    QApplication::restoreOverrideCursor();
    statusMessage(tr("PageRank Prestige indices saved as: ")+ fn);
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
    QString fn = appSettings["dataDir"] + "socnetv-report-centrality_proximity_prestige.dat";

    askAboutWeights();

    statusMessage(  QString(tr(" Please wait...")));
    createProgressBar();
    activeGraph.writePrestigeProximity(fn, true, false ,
                                       filterIsolateNodesAct->isChecked());
    destroyProgressBar();

    statusMessage( QString(tr(" displaying file...")));

    TextEditor *ed = new TextEditor(fn);        //OPEN A TEXT EDITOR WINDOW
    ed->show();
    QApplication::restoreOverrideCursor();
    statusMessage(tr("Proximity Prestige Centralities saved as: ")+ fn);
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
                   tr("Please note that this function is <b>SLOW</b> on large "
                      "networks (n>200), since it will calculate  a (n x n) matrix A with:"
                      "Aii=1+weighted_degree_ni"
                      "Aij=1 if (i,j)=0"
                      "Aij=1-wij if (i,j)=wij\n"
                      "Next, it will compute the inverse matrix C of A."
                      "The computation of the inverse matrix is a CPU intensive function."
                      "although it uses LU decomposition.\n\n "
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
    QString fn = appSettings["dataDir"] + "socnetv-report-centrality_information.dat";
    statusMessage(  QString(tr(" Please wait...")));

    askAboutWeights();
    createProgressBar();
    activeGraph.writeCentralityInformation(fn,considerWeights, inverseWeights);
    destroyProgressBar();

    TextEditor *ed = new TextEditor(fn);
    ed->show();
    QApplication::restoreOverrideCursor();
    statusMessage(tr("Information Centralities saved as: ")+ fn);
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
    QString fn = appSettings["dataDir"] + "socnetv-report-centrality_stress.dat";


    askAboutWeights();

    statusMessage(  QString(tr(" Please wait...")));
    createProgressBar();
    activeGraph.writeCentralityStress(
                fn, considerWeights, inverseWeights,
                filterIsolateNodesAct->isChecked());
    destroyProgressBar();

    TextEditor *ed = new TextEditor(fn);        //OPEN A TEXT EDITOR WINDOW
    ed->show();
    QApplication::restoreOverrideCursor();
    statusMessage(tr("Stress Centralities saved as: ")+ fn);
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
    QString fn = appSettings["dataDir"] + "socnetv-report-centrality_power.dat";


    askAboutWeights();

    statusMessage(  QString(tr(" Please wait...")));
    createProgressBar();
    activeGraph.writeCentralityPower(
                fn, considerWeights, inverseWeights,
                filterIsolateNodesAct->isChecked());
    destroyProgressBar();

    TextEditor *ed = new TextEditor(fn);        //OPEN A TEXT EDITOR WINDOW
    ed->show();
    QApplication::restoreOverrideCursor();
    statusMessage(tr("Stress Centralities saved as: ")+ fn);
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
    QString fn = appSettings["dataDir"] + "socnetv-report-centrality_eccentricity.dat";


    askAboutWeights();

    statusMessage(  QString(tr(" Please wait...")));
    createProgressBar();
    activeGraph.writeCentralityEccentricity(
                fn, considerWeights, inverseWeights,
                filterIsolateNodesAct->isChecked());
    destroyProgressBar();

    TextEditor *ed = new TextEditor(fn);        //OPEN A TEXT EDITOR WINDOW
    ed->show();
    QApplication::restoreOverrideCursor();
    statusMessage(tr("Eccentricity Centralities saved as: ")+ fn);
}



/**
 * @brief MainWindow::createProgressBar
 * @param max
 * @param msg
 * Creates a Qt Progress Dialog
 * if max = 0, then max becomes equal to active vertices*
 */
void MainWindow::createProgressBar(int max, QString msg){

    qDebug() << "MW::createProgressBar" ;
    if ( max == 0 ) {
            max = activeGraph.vertices();
        }

    if ( ( appSettings["showProgressBar"] == "true"  && max > 100 )
         || activeEdges() > 2000 ){
        progressDialog= new QProgressDialog(msg, "Cancel", 0, max, this);
        progressDialog -> setWindowModality(Qt::WindowModal);
        connect( &activeGraph, SIGNAL( updateProgressDialog(int) ),
                 progressDialog, SLOT(setValue(int) ) ) ;
        progressDialog->setMinimumDuration(0);
    }

    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

}


/**
 * @brief MainWindow::destroyProgressBar
 */
void MainWindow::destroyProgressBar(){
    qDebug () << "MainWindow::destroyProgressBar";
    QApplication::restoreOverrideCursor();
    qDebug () << "MainWindow::destroyProgressBar - check if a progressbar exists";
    if ( ( appSettings["showProgressBar"] == "true" || activeEdges() > 2000 ) && progressDialog) {
        qDebug () << "MainWindow::destroyProgressBar - progressbar exists. Dstroying";
        //progressDialog->hide();
    }


}





/**
 * @brief MainWindow::slotOptionsNodeNumbersVisibility
 * Turns on/off displaying the numbers of nodes (outside ones)
 * @param toggle
 */
void MainWindow::slotOptionsNodeNumbersVisibility(bool toggle) {
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
 * @brief MainWindow::slotOptionsNodeNumbersInside
 * Turns on/off displaying the nodenumbers inside the nodes.
 * @param toggle
 */
void MainWindow::slotOptionsNodeNumbersInside(bool toggle){
    statusMessage( tr("Toggle Numbers inside nodes. Please wait...") );

    if ( appSettings["initNodeNumbersVisibility"] != "true" )  	{
        // ?
    }
    else{
        slotOptionsNodeNumbersVisibility(true);
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
 * @brief MainWindow::slotOptionsNodeLabelsVisibility
 * Turns on/off displaying labels
 * @param toggle
 */
void MainWindow::slotOptionsNodeLabelsVisibility(bool toggle){
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
 * @brief MainWindow::slotOptionsEdgeWeightNumbersVisibility
 * Turns on/off displaying edge weight numbers
 * @param toggle
 */
void MainWindow::slotOptionsEdgeWeightNumbersVisibility(bool toggle) {
    if (!fileLoaded && ! networkModified) {
        QMessageBox::critical(this, "Error",tr("There are no edges! \nLoad a network file or create a new network first."), "OK",0);
        statusMessage( tr("No nodes or edges found. Sorry...") );
        return;
    }
    qDebug() << "MW::slotOptionsEdgeWeightNumbersVisibility - Toggling Edges Weights. Please wait...";
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
 * @brief MainWindow::slotOptionsEdgeWeightsDuringComputation
 * @param toggle
 */
void MainWindow::slotOptionsEdgeWeightsDuringComputation(bool toggle) {
   if (toggle) {
       considerWeights=true;
       askedAboutWeights=false;
        askAboutWeights(); // will only ask about inversion
   }
   else
       considerWeights=false;
}



/**
 * @brief MainWindow::slotOptionsEdgesVisibility
 * @param toggle
 */
void MainWindow::slotOptionsEdgesVisibility(bool toggle){
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
void MainWindow::slotOptionsEdgeArrrowsVisibility(bool toggle){
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
void MainWindow::slotOptionsEdgesBezier(bool toggle){
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
 * @brief MainWindow::slotOptionsEdgesThicknessPerWeight
 * @param toggle
 */
void MainWindow::slotOptionsEdgesThicknessPerWeight(bool toggle) {

}


/**
*  Changes the background color of the scene
*/
void MainWindow::slotBackgroundColor () {
    qDebug() << "MW: slotBackgroundColor " << appSettings["initBackgroundColor"];
    graphicsWidget ->setBackgroundBrush(
                QBrush(QColor (appSettings["initBackgroundColor"]))
            );
    statusMessage( tr("Ready. ") );
}






/**
 * @brief MainWindow::slotOptionsAntialiasing
 * Turns antialiasing on or off
 * @param toggle
 */
void MainWindow::slotOptionsAntialiasing(bool toggle) {
    statusMessage( tr("Toggle anti-aliasing. This will take some time if the network is large (>500)...") );
    //Inform graphicsWidget about the change
    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
    graphicsWidget->setRenderHint(QPainter::Antialiasing, toggle);
    graphicsWidget->setRenderHint(QPainter::TextAntialiasing, toggle);
    graphicsWidget->setRenderHint(QPainter::SmoothPixmapTransform, toggle);
    QApplication::restoreOverrideCursor();
    if (!toggle) {
        statusMessage( tr("Anti-aliasing off.") );
        appSettings["antialiasing"] = "false";
    }
    else {
        appSettings["antialiasing"] = "true";
        statusMessage( tr("Anti-aliasing on.") );
    }

}


/**
 * @brief MainWindow::slotOptionsEmbedLogoExporting
 *
 * @param toggle
 */
void MainWindow::slotOptionsEmbedLogoExporting(bool toggle){
    if (!toggle) {
        statusMessage( tr("SocNetV logo print off.") );
        appSettings["printLogo"] = "false";
    }
    else {
        appSettings["printLogo"] = "true";
        statusMessage( tr("SocNetV logo print on.") );
    }
}

/**
 * @brief MainWindow::slotOptionsProgressBarVisibility
 * @param toggle
 * turn progressbar on or off
 */
void MainWindow::slotOptionsProgressBarVisibility(bool toggle) {
    statusMessage( tr("Toggle progressbar..."));
    if (!toggle)  {
        appSettings["showProgressBar"] = "false";
        statusMessage( tr("Progress bars off.") );
    }
    else   {
        appSettings["showProgressBar"] = "true";
        statusMessage( tr("Progress bars on.") );
    }
}



/**
 * @brief MainWindow::slotOptionsDebugMessages
 * @param toggle
 * Turns debugging messages on or off
 */
void MainWindow::slotOptionsDebugMessages(bool toggle){
    if (!toggle)   {
        appSettings["printDebug"] = "false";
        printDebug=false;
        statusMessage( tr("Debug messages off.") );
    }
    else  {
        appSettings["printDebug"] = "true";
        printDebug=true;
        statusMessage( tr("Debug messages on.") );
    }
}





/**
 * @brief MainWindow::slotOptionsBackgroundImageSelect
 * Toggles displaying a custom image in the background
 * If toggle = true, presents a dialog to select an image file
 * Called from app menu option
 * @param toggle
 */
void MainWindow::slotOptionsBackgroundImageSelect(bool toggle) {
    statusMessage( tr("Toggle BackgroundImage..."));
    QString m_fileName ;
    if (toggle == false)   {
        statusMessage( tr("BackgroundImage off.") );
        graphicsWidget->setBackgroundBrush(
                     QBrush(QColor (appSettings["initBackgroundColor"] ) )
                    );
    }
    else   {
        m_fileName = QFileDialog::getOpenFileName(
                    this, tr("Select one image"), getLastPath(),
                    tr("All (*);;PNG (*.png);;JPG (*.jpg)")
                    );
        if (m_fileName.isNull() )
            appSettings["initBackgroundImage"] = "";
        appSettings["initBackgroundImage"] = m_fileName;
        slotOptionsBackgroundImage();
    }
}



/**
 * @brief MainWindow::slotOptionsBackgroundImage
 * Enables/disables displaying a user-defined custom image in the background
 * Called from Settings Dialog and
 */
void MainWindow::slotOptionsBackgroundImage() {
    statusMessage( tr("Toggle BackgroundImage..."));
    if (appSettings["initBackgroundImage"].isEmpty())   {
        statusMessage( tr("BackgroundImage off.") );
        graphicsWidget->setBackgroundBrush(
                     QBrush(QColor (appSettings["initBackgroundColor"] ) )
                    );
    }
    else   {
        setLastPath(appSettings["initBackgroundImage"]);
        graphicsWidget->setBackgroundBrush(QImage(appSettings["initBackgroundImage"]));
        graphicsWidget->setCacheMode(QGraphicsView::CacheBackground);
        statusMessage( tr("BackgroundImage on.") );
    }


}




/**
 * @brief MainWindow::slotOptionsToolbarVisibility
 * @param toggle
 * Turns Toolbar on or off
 */
void MainWindow::slotOptionsToolbarVisibility(bool toggle) {
    statusMessage( tr("Toggle toolbar..."));
    if (toggle== false)   {
        toolBar->hide();
        appSettings["showToolBar"] = "false";
        statusMessage( tr("Toolbar off.") );
    }
    else  {
        toolBar->show();
        appSettings["showToolBar"] = "true";
        statusMessage( tr("Toolbar on.") );
    }
}




/**
 * @brief MainWindow::slotOptionsStatusBarVisibility
 * @param toggle
 * Turns Statusbar on or off
 */
void MainWindow::slotOptionsStatusBarVisibility(bool toggle) {
    statusMessage( tr("Toggle statusbar..."));

    if (toggle == false)   {
        statusBar()->hide();
        appSettings["showStatusBar"] = "false";
        statusMessage( tr("Status bar off.") );
    }
    else   {
        statusBar()->show();
        appSettings["showStatusBar"] = "true";
        statusMessage( tr("Status bar on.") );
    }

}


/**
 * @brief MainWindow::slotOptionsLeftPanelVisibility
 * @param toggle
 */
void MainWindow::slotOptionsLeftPanelVisibility(bool toggle) {
    statusMessage( tr("Toggle left panel..."));

    if (toggle == false)   {
        leftPanel->hide();
        appSettings["showLeftPanel"] = "false";
        statusMessage( tr("Left Panel off.") );
    }
    else   {
        leftPanel->show();
        appSettings["showLeftPanel"] = "true";
        statusMessage( tr("Left Panel on.") );
    }

}


/**
 * @brief MainWindow::slotOptionsRightPanelVisibility
 * @param toggle
 */
void MainWindow::slotOptionsRightPanelVisibility(bool toggle) {
    statusMessage( tr("Toggle left panel..."));

    if (toggle == false)   {
        rightPanel->hide();
        appSettings["showRightPanel"] = "false";
        statusMessage( tr("Right Panel off.") );
    }
    else   {
        rightPanel->show();
        appSettings["showRightPanel"] = "true";
        statusMessage( tr("Right Panel on.") );
    }

}



/**
*  Displays a random tip
*/
void MainWindow::slotHelpTips() {
    int randomTip=rand()%tipsCounter; //Pick a tip.
    QMessageBox::about( this, tr("Tip Of The Day"), tips[randomTip]);
}



/**
    Creates our tips.
*/
void MainWindow::slotHelpCreateTips(){
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
QString BUILD="Thu Nov 21 01:42:08 EEST 2015";
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



