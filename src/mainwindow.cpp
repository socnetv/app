/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 2.4
 Written in Qt

-                           mainwindow.cpp  -  description
                             -------------------
    copyright         : (C) 2005-2017 by Dimitris B. Kalamaras
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
#include "dialogfilteredgesbyweight.h"
#include "guide.h"
#include "vertex.h"
#include "dialogpreviewfile.h"
#include "dialogranderdosrenyi.h"
#include "dialograndsmallworld.h"
#include "dialograndscalefree.h"
#include "dialograndregular.h"
#include "dialogsettings.h"
#include "dialogsimilaritypearson.h"
#include "dialogsimilaritymatches.h"
#include "dialogclusteringhierarchical.h"
#include "dialogdissimilarities.h"




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

    initPanels();      //build the toolbox

    initWindowLayout();   //init the application window, set layout etc

    initSignalSlots();  //connect signals and slots between app components

    initApp();          //  load and initialise default app parameters

    // Check if user-provided network file on startup
    qDebug() << "MW::MainWindow() Checking if user provided file on startup...";
    if (!m_fileName.isEmpty()) {
        slotNetworkFileChoose( m_fileName );
    }

    graphicsWidget->setFocus();

    statusMessage( tr("Welcome to Social Network Visualizer, Version ")+VERSION);

}



/**
 * @brief Deletes variables on MW closing
 */
MainWindow::~MainWindow() {
    qDebug() << "MW::~MainWindow() Destruct function running...";

    initApp();
    delete printer;
    delete scene;
    delete graphicsWidget;

    foreach ( TextEditor *ed, m_textEditors) {
        ed->close();
        delete ed;
    }
    m_textEditors.clear();


    qDebug() << "MW::~MainWindow() Destruct function finished - bye!";
}




/**
  * @brief Initializes default (or user-defined) app settings
  *
  */
QMap<QString,QString> MainWindow::initSettings(){
    qDebug()<< "MW::initSettings";

    printDebug = false; // comment it to stop debug override

    // Create fortune cookies and tips
    createFortuneCookies();
    slotHelpCreateTips();

    // Call slotNetworkAvailableTextCodecs to setup a list of all supported codecs
    qDebug() << "MW::initSettings - calling slotNetworkAvailableTextCodecs" ;
    slotNetworkAvailableTextCodecs();

    qDebug() << "MW::initSettings - creating DialogPreviewFile object and setting codecs list" ;
    m_dialogPreviewFile = new DialogPreviewFile(this);
    m_dialogPreviewFile->setCodecList(codecs);

    connect (m_dialogPreviewFile, &DialogPreviewFile::loadNetworkFileWithCodec,
             this, &MainWindow::slotNetworkFileLoad );

    qDebug() << "MW::initSettings - creating default settings" ;
    settingsDir = QDir::homePath() +QDir::separator() + "socnetv-data" + QDir::separator() ;
    settingsFilePath = settingsDir + "settings.conf";

    // initially they are the same, but dataDir may be changed by the user
    QString dataDir= settingsDir ;

    maxNodes=5000;		//Max nodes used by createRandomNetwork dialogues

    // hard-coded initial settings to use only on first app load
    // when there are no user defined values
    appSettings["initNodeSize"]= "10";
    appSettings["initNodeColor"]="red";
    appSettings["initNodeShape"]="circle";

    appSettings["initNodeNumbersVisibility"] = "true";
    appSettings["initNodeNumberSize"]="0";
    appSettings["initNodeNumberColor"]="#333";
    appSettings["initNodeNumbersInside"] = "true";
    appSettings["initNodeNumberDistance"] = "2";

    appSettings["initNodeLabelsVisibility"] = "false";
    appSettings["initNodeLabelSize"]="6";
    appSettings["initNodeLabelColor"]="#00aa00";
    appSettings["initNodeLabelDistance"] = "6";

    appSettings["initEdgesVisibility"]="true";
    appSettings["initEdgeShape"]="line"; //bezier
    appSettings["initEdgeColor"]="black";
    appSettings["initEdgeColorNegative"]="red";
    appSettings["initEdgeColorZero"]="blue";
    appSettings["initEdgeArrows"]="true";
    appSettings["initEdgeThicknessPerWeight"]="true";
    appSettings["initEdgeWeightNumbersVisibility"]="false";
    appSettings["initEdgeWeightNumberSize"] = "7";
    appSettings["initEdgeWeightNumberColor"] = "#00aa00";
    appSettings["initEdgeLabelsVisibility"] = "false";
    appSettings["considerWeights"]="false";
    appSettings["inverseWeights"]="false";
    appSettings["askedAboutWeights"]="false";

    appSettings["initBackgroundColor"]="white"; //"gainsboro";
    appSettings["initBackgroundImage"]="";
    appSettings["printDebug"] = (printDebug) ? "true" : "false";
    appSettings["viewReportsInSystemBrowser"] = "true";
    appSettings["showProgressBar"] = "true";
    appSettings["showToolBar"] = "true";
    appSettings["showStatusBar"] = "true";
    appSettings["antialiasing"] = "true";
    appSettings["dataDir"]= dataDir ;
    appSettings["lastUsedDirPath"]= dataDir ;
    appSettings["showRightPanel"] = "true";
    appSettings["showLeftPanel"] = "true";
    appSettings["printLogo"] = "true";
    appSettings["initStatusBarDuration"] = "5000";
    appSettings["randomErdosEdgeProbability"] = "0.04";

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
                qDebug() << "  read setting: " <<  setting[0].simplified() << " = " << setting[1].simplified();
                if (setting[0].simplified().startsWith("recentFile_"))
                    recentFiles += setting[1].simplified();
                else
                    appSettings.insert (setting[0].simplified() , setting[1].simplified() );
            }
        }
        file.close();
    }
    qDebug () << "MW::initSettings() - Recent files count " << recentFiles.count() ;
    // restore user setting for debug messages
    printDebug = (appSettings["printDebug"] == "true") ? true:false;

    return appSettings;
}




/**
 * @brief Saves default (or user-defined) app settings
 */
void MainWindow::saveSettings() {
    qDebug () << "MW::saveSettings to "<< settingsFilePath;
    QFile file(settingsFilePath);
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

    QTextStream out(&file);
    qDebug()<< "MW::saveSettings - writing settings to settings file first ";
    QMap<QString, QString>::const_iterator it = appSettings.constBegin();
    while (it != appSettings.constEnd()) {
        qDebug() << "   setting: " <<  it.key() << " = " << it.value();
        out << it.key() << " = " << it.value() << endl;
        ++it;
    }


    // save recent files
    for (int i = 0 ; i < recentFiles.size() ; ++i) {
         out << "recentFile_"+ QString::number(i+1)
             << " = "
             << recentFiles.at(i) << endl;
    }

    file.close();

}




/**
 * @brief Opens the Settings & Preferences dialog
 */
void MainWindow::slotOpenSettingsDialog() {
    qDebug() << "MW::slotOpenSettingsDialog()";

    // build dialog

    m_settingsDialog = new DialogSettings( appSettings, this);

    connect( m_settingsDialog, &DialogSettings::saveSettings,
                     this, &MainWindow::saveSettings);

    connect( m_settingsDialog, &DialogSettings::setDebugMsgs,
                         this, &MainWindow::slotOptionsDebugMessages);

    connect( m_settingsDialog, &DialogSettings::setProgressDialog,
             this, &MainWindow::slotOptionsProgressDialogVisibility);

    connect( m_settingsDialog, &DialogSettings::setAntialiasing,
             this, &MainWindow::slotOptionsAntialiasing);

    connect( m_settingsDialog, &DialogSettings::setPrintLogo,
                 this, &MainWindow::slotOptionsEmbedLogoExporting);

    connect( m_settingsDialog, &DialogSettings::setBgColor,
                     this, &MainWindow::slotOptionsBackgroundColor);

    connect( m_settingsDialog, &DialogSettings::setBgImage,
                     this, &MainWindow::slotOptionsBackgroundImage);

    connect( m_settingsDialog, &DialogSettings::setToolBar,
             this, &MainWindow::slotOptionsToolbarVisibility);

    connect( m_settingsDialog, &DialogSettings::setStatusBar,
             this, &MainWindow::slotOptionsStatusBarVisibility);

    connect( m_settingsDialog, &DialogSettings::setLeftPanel,
             this, &MainWindow::slotOptionsLeftPanelVisibility);

    connect( m_settingsDialog, &DialogSettings::setRightPanel,
             this, &MainWindow::slotOptionsRightPanelVisibility);

    connect(m_settingsDialog, SIGNAL(setNodeColor(QColor)),
            this, SLOT(slotEditNodeColorAll(QColor)) );

    connect( m_settingsDialog, &DialogSettings::setNodeShape,
             this, &MainWindow::slotEditNodeShape);

    connect( m_settingsDialog, &DialogSettings::setNodeSize,
             this, &MainWindow::slotEditNodeSizeAll);

    connect( m_settingsDialog, &DialogSettings::setNodeNumbersVisibility,
             this, &MainWindow::slotOptionsNodeNumbersVisibility);

    connect( m_settingsDialog, &DialogSettings::setNodeNumbersInside,
             this, &MainWindow::slotOptionsNodeNumbersInside);

    connect( m_settingsDialog, &DialogSettings::setNodeNumberColor,
             this, &MainWindow::slotEditNodeNumbersColor);

    connect( m_settingsDialog, &DialogSettings::setNodeNumberSize,
             this, &MainWindow::slotEditNodeNumberSize);

    connect( m_settingsDialog, &DialogSettings::setNodeNumberDistance,
             this, &MainWindow::slotEditNodeNumberDistance);

    connect( m_settingsDialog, &DialogSettings::setNodeLabelsVisibility,
             this, &MainWindow::slotOptionsNodeLabelsVisibility);

    connect( m_settingsDialog, &DialogSettings::setNodeLabelSize,
             this, &MainWindow::slotEditNodeLabelSize);

    connect( m_settingsDialog, &DialogSettings::setNodeLabelColor,
             this, &MainWindow::slotEditNodeLabelsColor);

    connect( m_settingsDialog, &DialogSettings::setNodeLabelDistance,
             this, &MainWindow::slotEditNodeLabelDistance);

    connect( m_settingsDialog, &DialogSettings::setEdgesVisibility,
             this, &MainWindow::slotOptionsEdgesVisibility);

    connect( m_settingsDialog, &DialogSettings::setEdgeArrowsVisibility,
             this, &MainWindow::slotOptionsEdgeArrowsVisibility);

    connect( m_settingsDialog, &DialogSettings::setEdgeColor,
             this, &MainWindow::slotEditEdgeColorAll);

    connect( m_settingsDialog, &DialogSettings::setEdgeWeightNumbersVisibility,
             this, &MainWindow::slotOptionsEdgeWeightNumbersVisibility);

    connect( m_settingsDialog, &DialogSettings::setEdgeLabelsVisibility,
             this, &MainWindow::slotOptionsEdgeLabelsVisibility);


    // show settings dialog
    m_settingsDialog->exec();

    qDebug ()<< appSettings["initBackgroundImage"] ;

}



/**
 * @brief Initializes all QActions of the application
 * Take a breath, the listing below is HUGE.
 */
void MainWindow::initActions(){
    printer = new QPrinter;

    /**
    Network menu actions
    */
    networkNew = new QAction(QIcon(":/images/new.png"), tr("&New"),  this);
    networkNew->setShortcut(Qt::CTRL+Qt::Key_N);
    networkNew->setStatusTip(tr("Create a new network"));
    networkNew->setToolTip(tr("New network"));
    networkNew->setWhatsThis(tr("New\n\n"
                                "Creates a new social network. "
                                "Firtst, checks if current network needs to be saved."));
    connect(networkNew, SIGNAL(triggered()), this, SLOT(slotNetworkNew()));

    networkOpen = new QAction(QIcon(":/images/open.png"), tr("&Open"), this);
    networkOpen->setShortcut(Qt::CTRL+Qt::Key_O);
    networkOpen->setToolTip(tr("Open network"));
    networkOpen->setStatusTip(tr("Open a GraphML formatted file of social network data."));
    networkOpen->setWhatsThis(tr("Open\n\n"
                              "Opens a file of a social network in GraphML format"));
    connect(networkOpen, SIGNAL(triggered()), this, SLOT(slotNetworkFileChoose()));


    for (int i = 0; i < MaxRecentFiles; ++i) {
         recentFileActs[i] = new QAction(this);
         recentFileActs[i]->setVisible(false);
         connect(recentFileActs[i], SIGNAL(triggered()),
                 this, SLOT(slotNetworkFileLoadRecent()));
     }

    networkImportGML = new QAction( QIcon(":/images/open.png"), tr("&GML"), this);
    networkImportGML->setStatusTip(tr("Import GML-formatted file"));
    networkImportGML->setWhatsThis(tr("Import GML\n\n"
                                 "Imports a social network from a GML-formatted file"));
    connect(networkImportGML, SIGNAL(triggered()), this, SLOT(slotNetworkImportGML()));


    networkImportPajek = new QAction( QIcon(":/images/open.png"), tr("&Pajek"), this);
    networkImportPajek->setStatusTip(tr("Import Pajek-formatted file"));
    networkImportPajek->setWhatsThis(tr("Import Pajek \n\n"
                                 "Imports a social network from a Pajek-formatted file"));
    connect(networkImportPajek, SIGNAL(triggered()), this, SLOT(slotNetworkImportPajek()));


    networkImportSM = new QAction( QIcon(":/images/open.png"), tr("&Adjacency Matrix"), this);
    networkImportSM->setStatusTip(tr("Import Adjacency matrix"));
    networkImportSM->setWhatsThis(tr("Import Sociomatrix \n\n"
                              "Imports a social network from an Adjacency matrix-formatted file"));
    connect(networkImportSM, SIGNAL(triggered()), this, SLOT(slotNetworkImportSM()));

    networkImportDot = new QAction( QIcon(":/images/open.png"), tr("GraphViz (.dot)"), this);
    networkImportDot->setStatusTip(tr("Import dot file"));
    networkImportDot->setWhatsThis(tr("Import GraphViz \n\n"
                               "Imports a social network from an GraphViz formatted file"));
    connect(networkImportDot, SIGNAL(triggered()),
            this, SLOT(slotNetworkImportDot()));


    networkImportDL = new QAction( QIcon(":/images/open.png"), tr("UCINET (.dl)..."), this);
    networkImportDL->setStatusTip(tr("ImportDL-formatted file (UCINET)"));
    networkImportDL->setWhatsThis(tr("Import UCINET\n\n"
                                     "Imports social network data from a DL-formatted file"));
    connect(networkImportDL, SIGNAL(triggered()), this, SLOT(slotNetworkImportDL()));


    networkImportList = new QAction( QIcon(":/images/open.png"), tr("&Edge list"), this);
    networkImportList->setStatusTip(tr("Import an edge list file. "));
    networkImportList->setWhatsThis(
                tr("Import edge list\n\n"
                   "Import a network from an edgelist file. "
                   "SocNetV supports EdgeList files with edge weights "
                   "as well as simple EdgeList files where the edges are non-value (see manual)"
                                ));
    connect(networkImportList, SIGNAL(triggered()),
            this, SLOT(slotNetworkImportEdgeList()));


    networkImportTwoModeSM = new QAction( QIcon(":/images/open.png"), tr("&Two Mode Sociomatrix"), this);
    networkImportTwoModeSM->setStatusTip(tr("Import two-mode sociomatrix (affiliation network) file"));
    networkImportTwoModeSM->setWhatsThis(tr("Import Two-Mode Sociomatrix \n\n"
                                     "Imports a two-mode network from a sociomatrix file. "
                                     "Two-mode networks are described by affiliation "
                                     "network matrices, where A(i,j) codes the "
                                     "events/organizations each actor is affiliated with."));
    connect(networkImportTwoModeSM, SIGNAL(triggered()),
            this, SLOT(slotNetworkImportTwoModeSM()));


    networkSave = new QAction(QIcon(":/images/save.png"), tr("&Save"),  this);
    networkSave->setShortcut(Qt::CTRL+Qt::Key_S);
    networkSave->setStatusTip(tr("Save social network to a file"));
    networkSave->setWhatsThis(tr("Save.\n\n"
                              "Saves the social network to file"));
    connect(networkSave, SIGNAL(triggered()), this, SLOT(slotNetworkSave()));

    networkSaveAs = new QAction(QIcon(":/images/save.png"), tr("Save &As..."),  this);
    networkSaveAs->setShortcut(Qt::CTRL+Qt::SHIFT+Qt::Key_S);
    networkSaveAs->setStatusTip(tr("Save network under a new filename"));
    networkSaveAs->setWhatsThis(tr("Save As\n\n"
                                "Saves the social network under a new filename"));
    connect(networkSaveAs, SIGNAL(triggered()), this, SLOT(slotNetworkSaveAs()));

    networkExportBMP = new QAction(QIcon(":/images/image.png"), tr("&BMP..."), this);
    networkExportBMP->setStatusTip(tr("Export social network to BMP image"));
    networkExportBMP->setWhatsThis(tr("Export BMP\n\n"
                                      "Exports the social network to a BMP image"));
    connect(networkExportBMP, SIGNAL(triggered()), this, SLOT(slotNetworkExportBMP()));

    networkExportPNG = new QAction( QIcon(":/images/image.png"), tr("&PNG..."), this);
    networkExportPNG->setStatusTip(tr("Export social network to PNG image"));
    networkExportPNG->setWhatsThis(tr("Export PNG \n\n"
                                      "Exports the social network to a PNG image"));
    connect(networkExportPNG, SIGNAL(triggered()), this, SLOT(slotNetworkExportPNG()));


    networkExportPDF = new QAction( QIcon(":/images/pdf.png"), tr("&PDF..."), this);
    networkExportPDF->setStatusTip(tr("Export social network to PDF"));
    networkExportPDF->setWhatsThis(tr("Export PDF\n\n"
                                      "Exports the social network to a PDF document"));
    connect(networkExportPDF, SIGNAL(triggered()), this, SLOT(slotNetworkExportPDF()));

    networkExportSM = new QAction( QIcon(":/images/save.png"), tr("&Adjacency Matrix"), this);
    networkExportSM->setStatusTip(tr("Export social network to an adjacency/sociomatrix file"));
    networkExportSM->setWhatsThis(tr("Export network to Adjacency format\n\n"
                              "Exports the social network to an "
                              "adjacency matrix-formatted file"));
    connect(networkExportSM, SIGNAL(triggered()), this, SLOT(slotNetworkExportSM()));

    networkExportPajek = new QAction( QIcon(":/images/save.png"), tr("&Pajek"), this);
    networkExportPajek->setStatusTip(tr("Export social network to a Pajek-formatted file"));
    networkExportPajek->setWhatsThis(tr("Export Pajek \n\n"
                                 "Exports the social network to a Pajek-formatted file"));
    connect(networkExportPajek, SIGNAL(triggered()), this, SLOT(slotNetworkExportPajek()));


    networkExportList = new QAction( QIcon(":/images/save.png"), tr("&List"), this);
    networkExportList->setStatusTip(tr("Export to List-formatted file. "));
    networkExportList->setWhatsThis(tr("Export List\n\n"
                                "Exports the network to a List-formatted file"));
    connect(networkExportList, SIGNAL(triggered()), this, SLOT(slotNetworkExportList()));

    networkExportDL = new QAction( QIcon(":/images/save.png"), tr("&DL..."), this);
    networkExportDL->setStatusTip(tr("Export network to UCINET-formatted file"));
    networkExportDL->setWhatsThis(tr("Export UCINET\n\n"
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
    networkPrint->setStatusTip(tr("Send the currrent social network to the printer"));
    networkPrint->setWhatsThis(tr("Print \n\n"
                                  "Sends whatever is viewable on "
                                  "the canvas to your printer. \n"
                                  "To print the whole social network, "
                                  "you might want to zoom-out."));
    connect(networkPrint, SIGNAL(triggered()), this, SLOT(slotNetworkPrint()));

    networkQuit = new QAction(QIcon(":/images/exit.png"), tr("E&xit"), this);
    networkQuit->setShortcut(Qt::CTRL+Qt::Key_Q);
    networkQuit->setStatusTip(tr("Quit SocNetV. Are you sure?"));
    networkQuit->setWhatsThis(tr("Exit\n\n"
                                 "Quits the application"));
    connect(networkQuit, SIGNAL(triggered()), this, SLOT(close()));


    openTextEditorAct = new QAction(QIcon(":/images/texteditor.png"),
                                    tr("Open Text Editor"),this);
    openTextEditorAct ->setShortcut(Qt::SHIFT+Qt::Key_F5);
    openTextEditorAct->setStatusTip(tr("Open a text editor "
                                       "to take notes, copy/paste network data, etc"));
    openTextEditorAct->setWhatsThis(tr("Text Editor\n\n"
                                       "Opens a simple text editor where you can "
                                       "copy paste network data, of any supported format, "
                                       "and save to a file. Then you can import that file to SocNetV..."));
    connect(openTextEditorAct, SIGNAL(triggered()), this, SLOT(slotNetworkTextEditor()));


    networkViewFileAct = new QAction(QIcon(":/images/networkfile.png"),
                                     tr("View Loaded File"),this);
    networkViewFileAct ->setShortcut(Qt::Key_F5);
    networkViewFileAct->setStatusTip(tr("Display the loaded social network file."));
    networkViewFileAct->setWhatsThis(tr("View Loaded File\n\n"
                                        "Displays the loaded social network file "));
    connect(networkViewFileAct, SIGNAL(triggered()), this, SLOT(slotNetworkFileView()));

    networkViewSociomatrixAct = new QAction(QIcon(":/images/sm.png"),
                                     tr("View Adjacency Matrix"),  this);
    networkViewSociomatrixAct ->setShortcut(Qt::Key_F6);
    networkViewSociomatrixAct->setStatusTip(tr("Display the adjacency matrix of the network."));
    networkViewSociomatrixAct->setWhatsThis(tr("View Adjacency Matrix\n\n"
                                        "Displays the adjacency matrix of the active network. \n\n"
                                        "The adjacency matrix of a social network is a matrix "
                                        "where each element a(i,j) is equal to the weight "
                                        "of the arc from actor (node) i to actor j. "
                                        "If the actors are not connected, then a(i,j)=0. "));
    connect(networkViewSociomatrixAct, SIGNAL(triggered()),
            this, SLOT(slotNetworkViewSociomatrix()));


    networkViewSociomatrixPlotAct = new QAction(QIcon(":/images/adjacencyplot.png"),
                                     tr("Plot Adjacency Matrix (text)"),  this);
    networkViewSociomatrixPlotAct ->setShortcut(Qt::SHIFT + Qt::Key_F6);
    networkViewSociomatrixPlotAct->setStatusTip(tr("Plots the adjacency matrix in a text file using unicode characters."));
    networkViewSociomatrixPlotAct->setWhatsThis(
                tr("Plot Adjacency Matrix (text)\n\n"
                   "Plots the adjacency matrix in a text file using "
                   "unicode characters. \n\n"
                   "In every element (i,j) of the \"image\", "
                   "a black square means actors i and j are connected"
                   "whereas a white square means they are disconnected."
                   ));
    connect(networkViewSociomatrixPlotAct, SIGNAL(triggered()),
            this, SLOT(slotNetworkViewSociomatrixPlotText()));


    networkDataSetSelectAct = new QAction(QIcon(":/images/petersengraph.png"),
                                     tr("Create From Known Data Sets"),  this);
    networkDataSetSelectAct ->setShortcut(Qt::Key_F7);
    networkDataSetSelectAct->setStatusTip(tr("Create a social network using one of the \'famous\' "
                                             "social network data sets included in SocNetV."));
    networkDataSetSelectAct->setWhatsThis(tr("Known Data Sets\n\n"
                                        "SocNetV includes a number of known "
                                        "(also called famous) data sets in Social Network Analysis, "
                                        "such as Krackhardt's high-tech managers, etc. "
                                             "Click this menu item or press F7 to select a data set.  "
                                        ""));
    connect(networkDataSetSelectAct, SIGNAL(triggered()),
            this, SLOT(slotNetworkDataSetSelect()));


    createErdosRenyiRandomNetworkAct = new QAction(QIcon(":/images/erdos.png"),
                                                   tr("Erdős–Rényi"),  this);
    createErdosRenyiRandomNetworkAct -> setShortcut(
                QKeySequence(Qt::CTRL + Qt::Key_R, Qt::CTRL + Qt::Key_E)
                );
    createErdosRenyiRandomNetworkAct->setStatusTip(tr("Create a random network "
                                                      "according to the Erdős–Rényi model"));
    createErdosRenyiRandomNetworkAct->setWhatsThis(
                tr("Erdős–Rényi \n\n"
                "Creates a random network either of G(n, p) model or G(n,M) model.\n"
                "In the first, edges are created with Bernoulli trials (probability p).\n"
                "In the second, a graph of exactly M edges is created."));
    connect(createErdosRenyiRandomNetworkAct, SIGNAL(triggered()),
            this, SLOT(slotNetworkRandomErdosRenyiDialog()));

    createLatticeNetworkAct = new QAction( QIcon(":/images/net1.png"),
                                           tr("Ring Lattice"), this);
    createLatticeNetworkAct -> setShortcut(
                QKeySequence(Qt::CTRL + Qt::Key_R, Qt::CTRL + Qt::Key_L)
                );
    createLatticeNetworkAct->setStatusTip(tr("Create a ring lattice random network."));
    createLatticeNetworkAct->setWhatsThis(
                tr("Ring Lattice \n\n")+
                tr("A ring lattice is a graph with N vertices each connected to d neighbors, d / 2 on each side."));
    connect(createLatticeNetworkAct, SIGNAL(triggered()), this, SLOT(slotNetworkRandomRingLattice()));

    createRegularRandomNetworkAct = new QAction(QIcon(":/images/net.png"), tr("d-Regular"), this);
    createRegularRandomNetworkAct -> setShortcut(
                        QKeySequence(Qt::CTRL + Qt::Key_R, Qt::CTRL + Qt::Key_R)
                        );
    createRegularRandomNetworkAct->setStatusTip(
                tr("Create a d-regular random network, where every actor has the same degree d."));
    createRegularRandomNetworkAct->setWhatsThis(
                tr("d-Regular \n\n") +
                tr("A random network where each actor has the same "
                   "number d of neighbours, aka the same degree d "));
    connect(createRegularRandomNetworkAct, SIGNAL(triggered()),
            this, SLOT(slotNetworkRandomRegularDialog()));

    createGaussianRandomNetworkAct = new QAction(tr("Gaussian"),	this);
    createGaussianRandomNetworkAct -> setShortcut(
                    QKeySequence(Qt::CTRL + Qt::Key_R, Qt::CTRL + Qt::Key_G)
                    );
    createGaussianRandomNetworkAct->setStatusTip(tr("Create a Gaussian distributed random network."));
    createGaussianRandomNetworkAct->setWhatsThis(tr("Gaussian \n\nCreates a random network of Gaussian distribution"));
    connect(createGaussianRandomNetworkAct, SIGNAL(triggered()), this, SLOT(slotNetworkRandomGaussian()));

    createSmallWorldRandomNetworkAct = new QAction(QIcon(":/images/sw.png"), tr("Small World"),	this);
    createSmallWorldRandomNetworkAct-> setShortcut(
                QKeySequence(Qt::CTRL + Qt::Key_R, Qt::CTRL + Qt::Key_W)
                );
    createSmallWorldRandomNetworkAct->setStatusTip(tr("Create a small-world random network."));
    createSmallWorldRandomNetworkAct ->
            setWhatsThis(
                tr("Small World \n\n") +
                tr("A Small World, according to the Watts and Strogatz model, "
                   "is a random network with short average path lengths and high clustering coefficient."));
    connect(createSmallWorldRandomNetworkAct, SIGNAL(triggered()), this, SLOT(slotNetworkRandomSmallWorldDialog()));

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
                   " SocNetV generates random scale-free networks according to the "
                   " Barabási–Albert (BA) model using a preferential attachment mechanism."));
    connect(createScaleFreeRandomNetworkAct, SIGNAL(triggered()),
            this, SLOT(slotNetworkRandomScaleFreeDialog()));



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
    editRelationNextAct->setStatusTip(tr("Load the next relation of the network (if any)."));
    editRelationNextAct->setWhatsThis(tr("Next Relation\n\nLoads the next relation of the network (if any)"));

    editRelationPreviousAct = new QAction(QIcon(":/images/prevrelation.png"),
                                      tr("Previous Relation"),  this);
    editRelationPreviousAct->setShortcut(Qt::ALT + Qt::Key_Left);
    editRelationPreviousAct->setToolTip(
                tr("Goto previous graph relation (ALT+Left)"));
    editRelationPreviousAct->setStatusTip(
                tr("Load the previous relation of the network (if any)."));
    editRelationPreviousAct->setWhatsThis(
                tr("Previous Relation\n\n"
                   "Loads the previous relation of the network (if any)"));

    editRelationAddAct = new QAction(QIcon(":/images/addrelation.png"),
                                      tr("Add New Relation"),  this);
    editRelationAddAct->setShortcut(Qt::ALT + Qt::CTRL + Qt::Key_N);
    editRelationAddAct->setToolTip(
                tr("Add a new relation to the active graph (Ctrl+Shift+N)"));
    editRelationAddAct->setStatusTip(
                tr("Add a new relation to the network. "
                   "Nodes will be preserved, edges will be removed. "));
    editRelationAddAct->setWhatsThis(
                tr("Add New Relation\n\n"
                   "Adds a new relation to the active network. "
                   "Nodes will be preserved, edges will be removed. "));

    editRelationRenameAct = new QAction(QIcon(":/images/edit-rename.png"),
                                  tr("Rename Relation"),  this);
    editRelationRenameAct->setToolTip(tr("Rename current relation"));
    editRelationRenameAct->setStatusTip(tr("Rename the current relation of the network (if any)."));
    editRelationRenameAct->setWhatsThis(tr("Rename Relation\n\n"
                                           "Renames the current relation of the network (if any)."));


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

    editNodeSelectNoneAct = new QAction(QIcon(":/images/selectnone.png"), tr("Deselect All"), this);
    editNodeSelectNoneAct->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_A));
    editNodeSelectNoneAct->setStatusTip(tr("Deselect all nodes"));
    editNodeSelectNoneAct->setWhatsThis(tr("Deselect all\n\n Clears the node selection"));
    connect(editNodeSelectNoneAct, SIGNAL(triggered()), this, SLOT(slotEditNodeSelectNone()));

    editNodeFindAct = new QAction(QIcon(":/images/find.png"), tr("Find Node"), this);
    editNodeFindAct->setShortcut(Qt::CTRL + Qt::Key_F);
    editNodeFindAct->setToolTip(tr("Find an actor by its number or label and highlight it. "
                                     "Press Ctrl+F again to undo."));
    editNodeFindAct->setStatusTip(tr("Find an actor by its number or label and highlight it. "
                                 "Press Ctrl+F again to undo."));
    editNodeFindAct->setWhatsThis(tr("Find Node\n\n"
                                     "Finds a node with a given number or label and "
                                     "highlights it by doubling its size. "
                                     "Ctrl+F again resizes back the node"));
    connect(editNodeFindAct, SIGNAL(triggered()), this, SLOT(slotEditNodeFind()) );

    editNodeAddAct = new QAction(QIcon(":/images/add.png"), tr("Add Node"), this);
    editNodeAddAct->setShortcut(Qt::CTRL + Qt::Key_Period);
    editNodeAddAct->setToolTip(
                tr("Add a new node to the network (Ctrl+.). \n\n"
                   "You can also create a new node \n"
                   "in a specific position by double-clicking.")
                );
    editNodeAddAct->setWhatsThis(
                tr("Add new node\n\n"
                   "Adds a new node to the network (Ctrl+.). \n\n"
                   "Alternately, you can create a new node "
                   "in a specific position by double-clicking "
                   "on that spot of the canvas.")
                );

    connect(editNodeAddAct, SIGNAL(triggered()), this, SLOT(slotEditNodeAdd()));

    editNodeRemoveAct = new QAction(QIcon(":/images/remove.png"),tr("Remove Node"), this);
    editNodeRemoveAct ->setShortcut(Qt::CTRL + Qt::ALT + Qt::Key_Period);
    //Single key shortcuts with backspace or del do no work in Mac http://goo.gl/7hz7Dx
    editNodeRemoveAct->setToolTip(tr("Remove selected node(s). \n\n"
                                     "If no nodes are selected, you will be prompted "
                                     "for a node number. "));

    editNodeRemoveAct->setStatusTip(tr("Remove selected node(s). If no nodes are selected, "
                                       "you will be prompted for a node number. "));
    editNodeRemoveAct->setWhatsThis(
                tr("Remove node\n\n"
                   "Removes selected node(s) from the network (Ctrl+Alt+.). \n"
                   "Alternately, you can remove a node by right-clicking on it. \n"
                   "If no nodes are selected, you will be prompted for a node number. ")
                );

    connect(editNodeRemoveAct, SIGNAL(triggered()), this, SLOT(slotEditNodeRemove()));

    editNodePropertiesAct = new QAction(QIcon(":/images/properties.png"),tr("Selected Node Properties"), this);
    editNodePropertiesAct ->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_Period );
    editNodePropertiesAct->setToolTip(tr("Change the basic properties of the selected node(s) \n\n"
                                           "There must be some nodes on the canvas!"));
    editNodePropertiesAct->setStatusTip(tr("Change the basic properties of the selected node(s) -- "
                                           "There must be some nodes on the canvas!"));
    editNodePropertiesAct->setWhatsThis(tr("Selected Node Properties\n\n"
                                           "If there are some nodes on the canvas, "
                                           " opens a properties dialog to edit "
                                           "their label, size, color, shape etc. \n"
                                           "You must have some node selected."));
    connect(editNodePropertiesAct, SIGNAL(triggered()), this, SLOT(slotEditNodePropertiesDialog()));


    editNodeSelectedToCliqueAct = new QAction(QIcon(":/images/cliquenew.png"),
                                               tr("Create a clique from selected nodes "), this);
    editNodeSelectedToCliqueAct ->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_X, Qt::CTRL + Qt::Key_C));
    editNodeSelectedToCliqueAct->setStatusTip(tr("Connect all selected nodes with edges to create a clique -- "
                                           "There must be some nodes selected!"));
    editNodeSelectedToCliqueAct->setWhatsThis(tr("Clique from Selected Nodes\n\n"
                                           "Adds all possible edges between selected nodes, "
                                           "so that they become a complete subgraph (clique)\n"
                                           "You must have some nodes selected."));
    connect(editNodeSelectedToCliqueAct, SIGNAL(triggered()),
            this, SLOT(slotEditNodeSelectedToClique()));


    editNodeSelectedToStarAct = new QAction(QIcon(":/images/subgraphstar.png"),
                                               tr("Create a star from selected nodes "), this);
    editNodeSelectedToStarAct ->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_X, Qt::CTRL + Qt::Key_S));
    editNodeSelectedToStarAct->setStatusTip(tr("Connect selected nodes with edges/arcs to create a star -- "
                                           "There must be some nodes selected!"));
    editNodeSelectedToStarAct->setWhatsThis(tr("Star from Selected Nodes\n\n"
                                           "Adds edges between selected nodes, "
                                           "so that they become a star subgraph.\n"
                                           "You must have some nodes selected."));
    connect(editNodeSelectedToStarAct, SIGNAL(triggered()),
            this, SLOT(slotEditNodeSelectedToStar()));


    editNodeSelectedToCycleAct = new QAction(QIcon(":/images/subgraphcycle.png"),
                                               tr("Create a cycle from selected nodes "), this);
    editNodeSelectedToCycleAct ->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_X, Qt::CTRL + Qt::Key_Y));
    editNodeSelectedToCycleAct->setStatusTip(tr("Connect selected nodes with edges/arcs to create a star -- "
                                           "There must be some nodes selected!"));
    editNodeSelectedToCycleAct->setWhatsThis(tr("Cycle from Selected Nodes\n\n"
                                           "Adds edges between selected nodes, "
                                           "so that they become a cycle subgraph.\n"
                                           "You must have some nodes selected."));
    connect(editNodeSelectedToCycleAct, SIGNAL(triggered()),
            this, SLOT(slotEditNodeSelectedToCycle()));


    editNodeSelectedToLineAct = new QAction(QIcon(":/images/subgraphline.png"),
                                               tr("Create a line from selected nodes "), this);
    editNodeSelectedToLineAct ->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_X, Qt::CTRL + Qt::Key_Y));
    editNodeSelectedToLineAct->setStatusTip(tr("Connect selected nodes with edges/arcs to create a line-- "
                                           "There must be some nodes selected!"));
    editNodeSelectedToLineAct->setWhatsThis(tr("Line from Selected Nodes\n\n"
                                           "Adds edges between selected nodes, "
                                           "so that they become a line subgraph.\n"
                                           "You must have some nodes selected."));
    connect(editNodeSelectedToLineAct, SIGNAL(triggered()),
            this, SLOT(slotEditNodeSelectedToLine()));


    editNodeColorAll = new QAction(QIcon(":/images/nodecolor.png"), tr("Change All Nodes Color (this session)"),	this);
    editNodeColorAll->setStatusTip(tr("Choose a new color for all nodes (in this session only)."));
    editNodeColorAll->setWhatsThis(tr("Nodes Color\n\n"
                                      "Changes all nodes color at once. \n"
                                      "This setting will apply to this session only. \n"
                                      "To permanently change it, use Settings & Preferences"));
    connect(editNodeColorAll, SIGNAL(triggered()), this, SLOT(slotEditNodeColorAll()) );

    editNodeSizeAllAct = new QAction(QIcon(":/images/resize.png"), tr("Change All Nodes Size (this session)"),	this);
    editNodeSizeAllAct->setStatusTip(tr("Change the size of all nodes (in this session only)"));
    editNodeSizeAllAct->setWhatsThis(tr("Change All Nodes Size\n\n"
                                        "Click to select and apply a new size for all nodes at once. \n"
                                        "This setting will apply to this session only. \n"
                                        "To permanently change it, use Settings & Preferences"));
    connect(editNodeSizeAllAct, SIGNAL(triggered()), this, SLOT(slotEditNodeSizeAll()) );

    editNodeShapeAll = new QAction(QIcon(":/images/nodeshape.png"), tr("Change All Nodes Shape (this session)"),	this);
    editNodeShapeAll->setStatusTip(tr("Change the shape of all nodes (this session only)"));
    editNodeShapeAll->setWhatsThis(tr("Change All Nodes Shape\n\n"
                                      "Click to select and apply a new shape for all nodes at once."
                                      "This setting will apply to this session only. \n"
                                      "To permanently change it, use Settings & Preferences"));
    connect(editNodeShapeAll, SIGNAL(triggered()), this, SLOT(slotEditNodeShape()) );


    editNodeNumbersSizeAct = new QAction(QIcon(":/images/nodenumbersize.png"),
                                         tr("Change All Node Numbers Size (this session)"),	this);
    editNodeNumbersSizeAct->setStatusTip(tr("Change the font size of the numbers of all nodes"
                                            "(in this session only)"));
    editNodeNumbersSizeAct->setWhatsThis(tr("Change Node Numbers Size\n\n"
                                            "Click to select and apply a new font size for all node numbers"
                                            "This setting will apply to this session only. \n"
                                            "To permanently change it, use Settings & Preferences"));
    connect(editNodeNumbersSizeAct, SIGNAL(triggered()),
            this, SLOT( slotEditNodeNumberSize(  )) );


    editNodeNumbersColorAct = new QAction(QIcon(":/images/nodenumbercolor.png"),
                                          tr("Change All Node Numbers Color (this session)"),	this);
    editNodeNumbersColorAct->setStatusTip(tr("Change the color of the numbers of all nodes."
                                              "(in this session only)"));
    editNodeNumbersColorAct->setWhatsThis(tr("Node Numbers Color\n\n"
                                              "Click to select and apply a new color "
                                              "to all node numbers."
                                              "This setting will apply to this session only. \n"
                                              "To permanently change it, use Settings & Preferences"));
    connect(editNodeNumbersColorAct, SIGNAL(triggered()), this, SLOT(slotEditNodeNumbersColor()));

    editNodeLabelsSizeAct = new QAction(QIcon(":/images/nodelabelsize.png"), tr("Change All Node Labels Size (this session)"), this);
    editNodeLabelsSizeAct->setStatusTip(tr("Change the font size of the labels of all nodes"
                                           "(this session only)"));
    editNodeLabelsSizeAct->setWhatsThis(tr("Node Labels Size\n\n"
                                           "Click to select and apply a new font-size to all node labels"
                                           "This setting will apply to this session only. \n"
                                           "To permanently change it, use Settings & Preferences"));
    connect(editNodeLabelsSizeAct, SIGNAL(triggered()), this, SLOT(slotEditNodeLabelSize()) );

    editNodeLabelsColorAct = new QAction(QIcon(":/images/nodelabelcolor.png"), tr("Change All Node Labels Color (this session)"),	this);
    editNodeLabelsColorAct->setStatusTip(tr("Change the color of the labels of all nodes "
                                             "(for this session only)"));
    editNodeLabelsColorAct->setWhatsThis(tr("Labels Color\n\n"
                                             "Click to select and apply a new color to all node labels."
                                             "This setting will apply to this session only. \n"
                                             "To permanently change it, use Settings & Preferences"));
    connect(editNodeLabelsColorAct, SIGNAL(triggered()), this, SLOT(slotEditNodeLabelsColor()));

    editEdgeAddAct = new QAction(QIcon(":/images/connect.png"), tr("Add Edge (arc)"),this);
    editEdgeAddAct->setShortcut(Qt::CTRL + Qt::Key_Slash);
    editEdgeAddAct->setStatusTip(tr("Add a directed edge (arc) from a node to another"));
    editEdgeAddAct->setToolTip(
                    tr("Add a new edge from a node to another (Ctrl+/).\n\n"
                       "You can also create an edge between two nodes \n"
                       "by double-clicking or middle-clicking on them consecutively."));
    editEdgeAddAct->setWhatsThis(
                tr("Add edge\n\n"
                   "Adds a new edge from a node to another (Ctrl+/).\n\n"
                   "Alternately, you can create a new edge between two nodes "
                   "by double-clicking or middle-clicking on them consecutively.")
                );
    connect(editEdgeAddAct, SIGNAL(triggered()), this, SLOT(slotEditEdgeAdd()));

    editEdgeRemoveAct = new QAction(QIcon(":/images/disconnect.png"), tr("Remove Edge"), this);
    editEdgeRemoveAct ->setShortcut(Qt::CTRL + Qt::ALT + Qt::Key_Slash);
    editEdgeRemoveAct ->setToolTip(tr("Remove selected edges from the network (Ctrl+Alt+/). \n\n"
                                      "If no edge has been clicked or selected, you will be prompted \n"
                                      "to enter edge source and target nodes for the edge to remove."));
    editEdgeRemoveAct->setStatusTip(tr("Remove selected Edge(s) (Ctrl+Alt+/)"));
    editEdgeRemoveAct->setWhatsThis(tr("Remove Edge\n\n"
                                       "Removes edges from the network (Ctrl+Alt+/). \n"
                                       "If one or more edges has been clicked or selected, they are removed. "
                                       "Otherwise, you will be prompted to enter edge source and target "
                                       "nodes for the edge to remove."));
    connect(editEdgeRemoveAct, SIGNAL(triggered()), this, SLOT(slotEditEdgeRemove()));

    editEdgeLabelAct = new QAction(QIcon(":/images/letters.png"), tr("Change Edge Label"), this);
    editEdgeLabelAct->setStatusTip(tr("Change the Label of an Edge"));
    editEdgeLabelAct->setWhatsThis(tr("Change Edge Label\n\n"
                                      "Changes the label of an Edge"));
    connect(editEdgeLabelAct, SIGNAL(triggered()), this, SLOT(slotEditEdgeLabel()));


    editEdgeColorAct = new QAction(QIcon(":/images/colorize.png"),tr("Change Edge Color"),	this);
    editEdgeColorAct->setStatusTip(tr("Change the Color of an Edge"));
    editEdgeColorAct->setWhatsThis(tr("Change Edge Color\n\n"
                                      "Changes the Color of an Edge"));
    connect(editEdgeColorAct, SIGNAL(triggered()), this, SLOT(slotEditEdgeColor()));

    editEdgeWeightAct = new QAction(QIcon(":/images/edgeweight.png") ,tr("Change Edge Weight"), this);
    editEdgeWeightAct->setStatusTip(tr("Change the weight of an Edge"));
    editEdgeWeightAct->setWhatsThis(tr("Edge Weight\n\n"
                                       "Changes the Weight of an Edge"));
    connect(editEdgeWeightAct, SIGNAL(triggered()), this, SLOT(slotEditEdgeWeight()));

    editEdgeColorAllAct = new QAction(QIcon(":/images/edgecolor.png"), tr("Change All Edges Color"), this);
    editEdgeColorAllAct->setStatusTip(tr("Change the color of all Edges."));
    editEdgeColorAllAct->setWhatsThis(tr("All Edges Color\n\n"
                                         "Changes the color of all Edges"));
    connect(editEdgeColorAllAct, SIGNAL(triggered()), this, SLOT(slotEditEdgeColorAll()));

    editEdgeSymmetrizeAllAct= new QAction(QIcon(":/images/symmetrize.png"), tr("Symmetrize Directed Edges"), this);
    editEdgeSymmetrizeAllAct ->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_E, Qt::CTRL + Qt::Key_S));
    editEdgeSymmetrizeAllAct->setStatusTip(tr("Make all arcs in this relation reciprocal (thus, a symmetric graph)."));
    editEdgeSymmetrizeAllAct->setWhatsThis(
                tr("Symmetrize Directed Edges\n\n"
                   "Makes all directed arcs in this relation reciprocal. \n"
                   "If there is an arc from node A to node B \n"
                   "then a new arc from node B to node A is created \n"
                   "with the same weight"
                   "The result is a symmetric network"));
    connect(editEdgeSymmetrizeAllAct, SIGNAL(triggered()), this, SLOT(slotEditEdgeSymmetrizeAll()));


    editEdgeSymmetrizeStrongTiesAct= new QAction(QIcon(":/images/symmetrize.png"), tr("Symmetrize Edges by Strong Ties"), this);
    editEdgeSymmetrizeStrongTiesAct ->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_E, Qt::CTRL + Qt::Key_T));
    editEdgeSymmetrizeStrongTiesAct->setStatusTip(tr("Create a new symmetric relation by counting reciprocated ties only (strong ties)."));
    editEdgeSymmetrizeStrongTiesAct->setWhatsThis(
                tr("Symmetrize Edges by examing Strong Ties\n\n"
                   "Creates a new symmetric relation by keeping strong ties only. \n"
                   "That is, a strong tie exists between actor A and actor B \n"
                   "only when both arcs A -> B and B -> A are present. \n"
                   "If the network is multi-relational, it asks you whether \n"
                   "ties in the current relation or all relations are to be considered. \n"
                   "The resulting relation is symmetric."));
    connect(editEdgeSymmetrizeStrongTiesAct, SIGNAL(triggered()),
            this, SLOT(slotEditEdgeSymmetrizeStrongTies()));


    //TODO Separate action for Directed/Undirected graph drawing (without changing all existing edges).
    editEdgeUndirectedAllAct= new QAction( tr("Undirected Edges"), this);
    editEdgeUndirectedAllAct ->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_E, Qt::CTRL + Qt::Key_U));
    editEdgeUndirectedAllAct->setStatusTip(tr("Enable to tranform all arcs to undirected edges and hereafter work with undirected edges ."));
    editEdgeUndirectedAllAct->setWhatsThis(
                tr("Undirected Edges\n\n"
                   "Tranforms all directed arcs to undirected edges. \n"
                   "The result is a undirected and symmetric network."
                   "After that, every new edge you add, will be undirected too."
                   "If you disable this, then all edges become directed again."));
    editEdgeUndirectedAllAct -> setCheckable(true);
    editEdgeUndirectedAllAct -> setChecked(false);
    connect(editEdgeUndirectedAllAct, SIGNAL(triggered(bool)),
            this, SLOT(slotEditEdgeUndirectedAll(bool)));



    editEdgesCocitationAct= new QAction(QIcon(":/images/symmetrize.png"), tr("Cocitation Network"), this);
    editEdgesCocitationAct ->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_E, Qt::CTRL + Qt::Key_C));
    editEdgesCocitationAct->setStatusTip(tr("Create a new symmetric relation by "
                                            "connecting actors that are cocitated by others."));
    editEdgesCocitationAct->setWhatsThis(
                tr("Symmetrize Edges by examing Strong Ties\n\n"
                   "Create a new symmetric relation by connecting actors "
                   "that are cocitated by others. \n"
                   "In the new relation, an edge will exist between actor i and "
                   "actor j only if C(i,j) > 0, where C the Cocitation Matrix. "
                   "Thus the actor pairs cited by more common neighbors will appear "
                   "with a stronger tie between them than pairs those cited by fewer "
                   "common neighbors. "
                   "The resulting relation is symmetric."));
    connect(editEdgesCocitationAct, SIGNAL(triggered()),
            this, SLOT(slotEditEdgeSymmetrizeCocitation()));


    transformNodes2EdgesAct = new QAction( tr("Transform Nodes to Edges"),this);
    transformNodes2EdgesAct->setStatusTip(tr("Transforms the network so that "
                                             "nodes become Edges and vice versa"));
    transformNodes2EdgesAct->setWhatsThis(tr("Transform Nodes EdgesAct\n\n"
                                             "Transforms network so that nodes become Edges and vice versa"));
    connect(transformNodes2EdgesAct, SIGNAL(triggered()),
            this, SLOT(slotEditTransformNodes2Edges()));



    filterNodesAct = new QAction(tr("Filter Nodes"), this);
    filterNodesAct -> setEnabled(false);
    //filterNodesAct->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_X, Qt::CTRL + Qt::Key_F));
    filterNodesAct->setStatusTip(tr("Filters Nodes of some value out of the network"));
    filterNodesAct->setWhatsThis(tr("Filter Nodes\n\n"
                                    "Filters Nodes of some value out of the network."));
    connect(filterNodesAct, SIGNAL(triggered()), this, SLOT(slotFilterNodes()));

    editFilterNodesIsolatesAct = new QAction(tr("Disable Isolate Nodes"), this);
    editFilterNodesIsolatesAct -> setEnabled(true);
    editFilterNodesIsolatesAct -> setCheckable(true);
    editFilterNodesIsolatesAct -> setChecked(false);
    editFilterNodesIsolatesAct -> setShortcut(QKeySequence(Qt::CTRL + Qt::Key_X, Qt::CTRL + Qt::Key_F));
    editFilterNodesIsolatesAct -> setStatusTip(tr("Temporarily filter out nodes with no edges"));
    editFilterNodesIsolatesAct -> setWhatsThis(tr("Filter Isolate Nodes\n\n"
                                             "Enables or disables displaying of isolate nodes. "
                                                  "Isolate nodes are those with no edges..."));
    connect(editFilterNodesIsolatesAct, SIGNAL(toggled(bool)),
            this, SLOT(slotEditFilterNodesIsolates(bool)));

    editFilterEdgesByWeightAct = new QAction(QIcon(":/images/filter.png"), tr("Filter Edges by Weight"), this);
    editFilterEdgesByWeightAct -> setEnabled(true);
    editFilterEdgesByWeightAct -> setShortcut(QKeySequence(Qt::CTRL + Qt::Key_E, Qt::CTRL + Qt::Key_F));
    editFilterEdgesByWeightAct -> setStatusTip(tr("Temporarily filter edges of some weight out of the network"));
    editFilterEdgesByWeightAct -> setWhatsThis(tr("Filter Edges\n\n"
                                      "Filters Edge of some specific weight out of the network."));
    connect(editFilterEdgesByWeightAct , SIGNAL(triggered()),
            this, SLOT(slotEditFilterEdgesByWeightDialog()));

    editFilterEdgesUnilateralAct = new QAction(tr("Disable unilateral edges"), this);
    editFilterEdgesUnilateralAct -> setEnabled(true);
    editFilterEdgesUnilateralAct -> setCheckable(true);
    editFilterEdgesUnilateralAct -> setChecked(false);
    editFilterEdgesUnilateralAct -> setShortcut(QKeySequence(Qt::CTRL + Qt::Key_E, Qt::CTRL + Qt::Key_R));
    editFilterEdgesUnilateralAct -> setStatusTip(tr("Temporarily disable all unilateral (non-reciprocal) edges in this relation. Keeps only \"strong\" ties."));
    editFilterEdgesUnilateralAct -> setWhatsThis(tr("Unilateral edges\n\n"
                                      "In directed networks, a tie between two actors "
                                      "is unilateral when only one actor identifies the other "
                                      "as connected (i.e. friend, vote, etc). "
                                      "A unilateral tie is depicted as a single arc. "
                                      "These ties are considered weak, as opposed to "
                                      "reciprocal ties where both actors identify each other as connected. "
                                      "Strong ties are depicted as either a single undirected edge "
                                      "or as two reciprocated arcs between two nodes. "
                                       "By selecting this option, all unilateral edges in this relation will be disabled."));
    connect(editFilterEdgesUnilateralAct , SIGNAL(triggered(bool)),
            this, SLOT(slotEditFilterEdgesUnilateral(bool)));




    /**
    Layout menu actions
    */
    strongColorationAct = new QAction ( tr("Strong Structural"), this);
    strongColorationAct -> setStatusTip( tr("Nodes are assigned the same color if they have identical in and out neighborhoods") );
    strongColorationAct -> setWhatsThis( tr("Click this to colorize nodes; Nodes are assigned the same color if they have identical in and out neighborhoods"));
    connect(strongColorationAct, SIGNAL(triggered() ), this, SLOT(slotLayoutColorationStrongStructural()) );

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
    connect(regularColorationAct, SIGNAL(triggered() ), this, SLOT(slotLayoutColorationRegular()) );//TODO

    layoutRandomAct = new QAction( tr("Random"),this);
    layoutRandomAct -> setShortcut(Qt::CTRL+Qt::SHIFT+Qt::Key_0);
    layoutRandomAct -> setStatusTip(tr("Layout the network actors in random positions."));
    layoutRandomAct -> setWhatsThis(tr("Random Layout\n\n "
                                     "This layout algorithm repositions all "
                                     "network actors in random positions."));
    connect(layoutRandomAct, SIGNAL(triggered()), this, SLOT(slotLayoutRandom()));


    layoutRandomRadialAct = new QAction(tr("Random Circles"),	this);
    layoutRandomRadialAct -> setShortcut(Qt::CTRL+Qt::ALT+Qt::Key_0);
    layoutRandomRadialAct ->setStatusTip(tr("Layout the network in random concentric circles"));
    layoutRandomRadialAct->
            setWhatsThis(
                tr("Random Circles Layout\n\n Repositions the nodes randomly on circles"));
    connect(layoutRandomRadialAct, SIGNAL(triggered()), this, SLOT(slotLayoutRadialRandom()));




    layoutRadialProminence_DC_Act = new QAction( tr("Degree Centrality"),	this);
    layoutRadialProminence_DC_Act -> setShortcut(Qt::CTRL + Qt::ALT+ Qt::Key_1);
    layoutRadialProminence_DC_Act
            ->setStatusTip(
                tr("Place all nodes on concentric circles of radius inversely "
                    "proportional to their Degree Centrality."));
    layoutRadialProminence_DC_Act->
            setWhatsThis(
                tr( "Degree Centrality (DC) Radial Layout\n\n"
                    "Repositions all nodes on concentric circles of radius "
                    "inversely proportional to their Degree Centrality score. "
                    "Nodes with higher DC are closer to the centre."
                    ));
    connect(layoutRadialProminence_DC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutRadialByProminenceIndex()) );

    layoutRadialProminence_CC_Act = new QAction( tr("Closeness Centrality"), this);
    layoutRadialProminence_CC_Act -> setShortcut(Qt::CTRL + Qt::ALT+ Qt::Key_2);
    layoutRadialProminence_CC_Act
            -> setStatusTip(
                tr("Place all nodes on concentric circles of radius inversely "
                    "proportional to their Closeness Centrality."));
    layoutRadialProminence_CC_Act->
            setWhatsThis(
                tr( "Closeness Centrality (CC) Radial Layout\n\n"
                    "Repositions all nodes on concentric circles of radius "
                    "inversely proportional to their Closeness Centrality. "
                    "Nodes having higher CC are closer to the centre."
                    ));
    connect(layoutRadialProminence_CC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutRadialByProminenceIndex()));


    layoutRadialProminence_IRCC_Act = new QAction(
                tr("Influence Range Closeness Centrality"),	this);
    layoutRadialProminence_IRCC_Act -> setShortcut(Qt::CTRL + Qt::ALT+ Qt::Key_3);
    layoutRadialProminence_IRCC_Act
            ->setStatusTip(
                tr("Place all nodes on concentric circles of radius inversely "
                    "proportional to their Influence Range Closeness Centrality."));
    layoutRadialProminence_IRCC_Act->
            setWhatsThis(
                tr("Influence Range Closeness Centrality (IRCC) Radial Layout\n\n"
                    "Repositions all nodes on concentric circles of radius "
                    "inversely proportional to their IRCC score. "
                    "Nodes having higher IRCC are closer to the centre."
                    ));
    connect(layoutRadialProminence_IRCC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutRadialByProminenceIndex()));

    layoutRadialProminence_BC_Act = new QAction( tr("Betweenness Centrality"), this);
    layoutRadialProminence_BC_Act -> setShortcut(Qt::CTRL + Qt::ALT+ Qt::Key_4);
    layoutRadialProminence_BC_Act ->setStatusTip(
                tr("Place all nodes on concentric circles of radius inversely "
                    "proportional to their Betweenness Centrality."));
    layoutRadialProminence_BC_Act->
            setWhatsThis(
                tr("Betweenness Centrality (BC) Radial Layout\n\n"
                    "Repositions all nodes on concentric circles of radius "
                    "inversely proportional to their Betweenness Centrality. "
                    "Nodes having higher BC are closer to the centre."
                    ));
    connect(layoutRadialProminence_BC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutRadialByProminenceIndex()));

    layoutRadialProminence_SC_Act = new QAction( tr("Stress Centrality"),	this);
    layoutRadialProminence_SC_Act -> setShortcut(Qt::CTRL + Qt::ALT+ Qt::Key_5);
    layoutRadialProminence_SC_Act ->setStatusTip(
                tr("Place all nodes on concentric circles of radius inversely "
                    "proportional to their Stress Centrality."));
    layoutRadialProminence_SC_Act->
            setWhatsThis(
                tr("Stress Centrality (SC) Radial Layout\n\n"
                    "Repositions all nodes on concentric circles of radius "
                    "inversely proportional to their Stress Centrality score. "
                    "Nodes having higher SC are closer to the centre."
                    ));
    connect(layoutRadialProminence_SC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutRadialByProminenceIndex()));

    layoutRadialProminence_EC_Act = new QAction( tr("Eccentricity Centrality"),	this);
    layoutRadialProminence_EC_Act -> setShortcut(Qt::CTRL + Qt::ALT+ Qt::Key_6);
    layoutRadialProminence_EC_Act ->setStatusTip(
                tr("Place all nodes on concentric circles of radius inversely "
                    "proportional to their Eccentricity Centrality."));
    layoutRadialProminence_EC_Act->
            setWhatsThis(
                tr("Eccentricity Centrality (EC) Radial Layout\n\n"
                    "Repositions all nodes on concentric circles of radius "
                    "inversely proportional to their Eccentricity Centrality score. "
                    "Nodes having higher EC are closer to the centre."
                    ));
    connect(layoutRadialProminence_EC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutRadialByProminenceIndex()));


    layoutRadialProminence_PC_Act = new QAction( tr("Power Centrality"),	this);
    layoutRadialProminence_PC_Act -> setShortcut(Qt::CTRL + Qt::ALT+ Qt::Key_7);
    layoutRadialProminence_PC_Act ->setStatusTip(
                tr("Place all nodes on concentric circles of radius inversely "
                    "proportional to their Power Centrality."));
    layoutRadialProminence_PC_Act->
            setWhatsThis(
                tr("Power Centrality (PC) Radial Layout\n\n"
                    "Repositions all nodes on concentric circles of radius "
                    "inversely proportional to their Power Centrality score. "
                    "Nodes having higher PC are closer to the centre."
                    ));
    connect(layoutRadialProminence_PC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutRadialByProminenceIndex()));


    layoutRadialProminence_IC_Act = new QAction( tr("Information Centrality"),	this);
    layoutRadialProminence_IC_Act -> setEnabled(true);
    layoutRadialProminence_IC_Act -> setShortcut(Qt::CTRL + Qt::ALT+ Qt::Key_8);
    layoutRadialProminence_IC_Act -> setStatusTip(
                tr("Place all nodes on concentric circles of radius inversely "
                    "proportional to their Information Centrality."));
    layoutRadialProminence_IC_Act->
            setWhatsThis(
                tr("Information Centrality (IC) Radial Layout\n\n"
                    "Repositions all nodes on concentric circles of radius "
                    "inversely proportional to their Information Centrality score. "
                    "Nodes of higher IC are closer to the centre."
                    ));
    connect(layoutRadialProminence_IC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutRadialByProminenceIndex()));


    layoutRadialProminence_EVC_Act = new QAction( tr("Eigenvector Centrality"),	this);
    layoutRadialProminence_EVC_Act -> setEnabled(true);
    layoutRadialProminence_EVC_Act -> setShortcut(Qt::CTRL + Qt::ALT+ Qt::Key_9);
    layoutRadialProminence_EVC_Act -> setStatusTip(
                tr("Place all nodes on concentric circles of radius inversely "
                    "proportional to their Eigenvector Centrality."));
    layoutRadialProminence_EVC_Act->
            setWhatsThis(
                tr("Eigenvector Centrality (EVC) Radial Layout\n\n"
                    "Repositions all nodes on concentric circles of radius "
                    "inversely proportional to their Eigenvector Centrality score. "
                    "Nodes of higher EVC are closer to the centre."
                    ));
    connect(layoutRadialProminence_EVC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutRadialByProminenceIndex()));


    layoutRadialProminence_DP_Act = new QAction( tr("Degree Prestige"),	this);
    layoutRadialProminence_DP_Act->setShortcut(Qt::CTRL + Qt::ALT+ Qt::Key_I);
    layoutRadialProminence_DP_Act ->setStatusTip(
                tr("Place all nodes on concentric circles of radius inversely "
                    "proportional to their Degree Prestige (inDegree)."));
    layoutRadialProminence_DP_Act->
            setWhatsThis(
                tr("Degree Prestige (DP) Radial Layout\n\n"
                    "Repositions all nodes on concentric circles of radius "
                    "inversely proportional to their inDegree score. "
                    "Nodes having higher DP are closer to the centre."
                    ));
    connect(layoutRadialProminence_DP_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutRadialByProminenceIndex()));

    layoutRadialProminence_PRP_Act = new QAction( tr("PageRank Prestige"),	this);
    layoutRadialProminence_PRP_Act ->setEnabled(true);
    layoutRadialProminence_PRP_Act->setShortcut(Qt::CTRL + Qt::ALT+ Qt::Key_K);
    layoutRadialProminence_PRP_Act ->setStatusTip(
                tr("Place all nodes on concentric circles of radius inversely "
                    "proportional to their PRP index."));
    layoutRadialProminence_PRP_Act->
            setWhatsThis(
                tr("PageRank Prestige (PRP) Radial Layout\n\n"
                    "Repositions all nodes on concentric circles of radius "
                    "inversely proportional to their PageRank score. "
                    "Nodes having higher PRP are closer to the centre."
                    ));
    connect(layoutRadialProminence_PRP_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutRadialByProminenceIndex()));


    layoutRadialProminence_PP_Act = new QAction( tr("Proximity Prestige"),	this);
    layoutRadialProminence_PP_Act ->setShortcut(Qt::CTRL + Qt::ALT+ Qt::Key_Y);
    layoutRadialProminence_PP_Act ->setStatusTip(
                tr("Place all nodes on concentric circles of radius inversely "
                    "proportional to their Proximity Prestige."));
    layoutRadialProminence_PP_Act->
            setWhatsThis(
                tr("Proximity Prestige (PP) Radial Layout\n\n"
                    "Repositions all nodes on concentric circles of radius "
                    "inversely proportional to their PP index. "
                    "Nodes having higher PP score are closer to the centre."
                    ));
    connect(layoutRadialProminence_PP_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutRadialByProminenceIndex()));




    layoutLevelProminence_DC_Act = new QAction( tr("Degree Centrality"), this);
    layoutLevelProminence_DC_Act -> setShortcut(Qt::CTRL + Qt::SHIFT+ Qt::Key_1);
    layoutLevelProminence_DC_Act
            ->setStatusTip(
                tr("Place all nodes on horizontal levels of height "
                     "proportional to their Degree Centrality."));
    layoutLevelProminence_DC_Act->
             setWhatsThis(
                 tr("Degree Centrality (DC) Levels Layout\n\n"
                     "Repositions all nodes on horizontal levels of height"
                     "proportional to their DC score. "
                     "Nodes having higher DC are closer to the top.\n\n"
                    )
                );
    connect(layoutLevelProminence_DC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutLevelByProminenceIndex()) );

    layoutLevelProminence_CC_Act = new QAction( tr("Closeness Centrality"), this);
    layoutLevelProminence_CC_Act -> setShortcut(Qt::CTRL + Qt::SHIFT+ Qt::Key_2);
    layoutLevelProminence_CC_Act
            -> setStatusTip(
                tr("Place all nodes on horizontal levels of height "
                     "proportional to their Closeness Centrality."));
    layoutLevelProminence_CC_Act->
             setWhatsThis(
                 tr("Closeness Centrality (CC) Levels Layout\n\n"
                     "Repositions all nodes on horizontal levels of height"
                     "proportional to their Closeness Centrality score. "
                     "Nodes of higher CC are closer to the top.\n\n"
                     "This layout can be computed only for connected graphs. "
                    ));
    connect(layoutLevelProminence_CC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutLevelByProminenceIndex()));


    layoutLevelProminence_IRCC_Act = new QAction(
                tr("Influence Range Closeness Centrality"),	this);
    layoutLevelProminence_IRCC_Act -> setShortcut(Qt::CTRL + Qt::SHIFT+ Qt::Key_3);
    layoutLevelProminence_IRCC_Act
            ->setStatusTip(
                tr("Place all nodes on horizontal levels of height "
                     "proportional to their Influence Range Closeness Centrality."));
    layoutLevelProminence_IRCC_Act->
             setWhatsThis(
                 tr("Influence Range Closeness Centrality (IRCC) Levels Layout\n\n"
                     "Repositions all nodes on horizontal levels of height"
                     "proportional to their IRCC score. "
                     "Nodes having higher IRCC are closer to the top.\n\n"
                     "This layout can be computed for not connected graphs. "
                    ));
    connect(layoutLevelProminence_IRCC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutLevelByProminenceIndex()));

    layoutLevelProminence_BC_Act = new QAction( tr("Betweenness Centrality"), this);
    layoutLevelProminence_BC_Act -> setShortcut(Qt::CTRL + Qt::SHIFT+ Qt::Key_4);
    layoutLevelProminence_BC_Act ->setStatusTip(
                tr("Place all nodes on horizontal levels of height "
                     "proportional to their Betweenness Centrality."));
    layoutLevelProminence_BC_Act->
             setWhatsThis(
                 tr("Betweenness Centrality (BC) Levels Layout\n\n"
                     "Repositions all nodes on horizontal levels of height"
                     "proportional to their Betweenness Centrality score. "
                     "Nodes having higher BC are closer to the top."
                    ));
    connect(layoutLevelProminence_BC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutLevelByProminenceIndex()));

    layoutLevelProminence_SC_Act = new QAction( tr("Stress Centrality"),	this);
    layoutLevelProminence_SC_Act -> setShortcut(Qt::CTRL + Qt::SHIFT+ Qt::Key_5);
    layoutLevelProminence_SC_Act ->setStatusTip(
                tr("Place nodes on horizontal levels of height "
                     "proportional to their Stress Centrality."));
    layoutLevelProminence_SC_Act->
             setWhatsThis(
                 tr("Stress Centrality (SC) Levels Layout\n\n"
                     "Repositions all nodes on horizontal levels of height"
                     "proportional to their Stress Centrality score. "
                     "Nodes having higher SC are closer to the top."
                    ));
    connect(layoutLevelProminence_SC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutLevelByProminenceIndex()));

    layoutLevelProminence_EC_Act = new QAction( tr("Eccentricity Centrality"),	this);
    layoutLevelProminence_EC_Act -> setShortcut(Qt::CTRL + Qt::SHIFT+ Qt::Key_6);
    layoutLevelProminence_EC_Act ->setStatusTip(
                tr("Place nodes on horizontal levels of height "
                     "proportional to their Eccentricity Centrality."));
    layoutLevelProminence_EC_Act->
             setWhatsThis(
                 tr("Eccentricity Centrality (EC) Levels Layout\n\n"
                     "Repositions all nodes on horizontal levels of height"
                     "proportional to their Eccentricity Centrality score. "
                     "Nodes having higher EC are closer to the top."
                    ));
    connect(layoutLevelProminence_EC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutLevelByProminenceIndex()));


    layoutLevelProminence_PC_Act = new QAction( tr("Power Centrality"),	this);
    layoutLevelProminence_PC_Act -> setShortcut(Qt::CTRL + Qt::SHIFT+ Qt::Key_7);
    layoutLevelProminence_PC_Act ->setStatusTip(
                tr("Place nodes on horizontal levels of height "
                     "proportional to their Power Centrality."));
    layoutLevelProminence_PC_Act->
             setWhatsThis(
                 tr("Power Centrality (PC) Levels Layout\n\n"
                     "Repositions all nodes on horizontal levels of height"
                     "proportional to their Power Centrality score. "
                     "Nodes having higher PC are closer to the top."
                    ));
    connect(layoutLevelProminence_PC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutLevelByProminenceIndex()));


    layoutLevelProminence_IC_Act = new QAction( tr("Information Centrality"),	this);
    layoutLevelProminence_IC_Act ->setEnabled(true);
    layoutLevelProminence_IC_Act -> setShortcut(Qt::CTRL + Qt::SHIFT+ Qt::Key_8);
    layoutLevelProminence_IC_Act ->setStatusTip(
                tr("Place nodes on horizontal levels of height "
                     "proportional to their Information Centrality."));
    layoutLevelProminence_IC_Act->
             setWhatsThis(
                 tr("Information Centrality (IC) Levels Layout\n\n"
                     "Repositions all nodes on horizontal levels of height"
                     "proportional to their Information Centrality score. "
                     "Nodes having higher IC are closer to the top."
                    ));
    connect(layoutLevelProminence_IC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutLevelByProminenceIndex()));

    layoutLevelProminence_EVC_Act = new QAction( tr("Eigenvector Centrality"),	this);
    layoutLevelProminence_EVC_Act ->setEnabled(true);
    layoutLevelProminence_EVC_Act -> setShortcut(Qt::CTRL + Qt::SHIFT+ Qt::Key_9);
    layoutLevelProminence_EVC_Act ->setStatusTip(
                tr(
                    "Place nodes on horizontal levels of height "
                     "proportional to their Eigenvector Centrality."));
    layoutLevelProminence_EVC_Act->
             setWhatsThis(
                 tr("Eigenvector Centrality (EVC) Levels Layout\n\n"
                     "Repositions all nodes on horizontal levels of height"
                     "proportional to their Eigenvector Centrality score. "
                     "Nodes having higher EVC are closer to the top."
                    ));
    connect(layoutLevelProminence_EVC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutLevelByProminenceIndex()));



    layoutLevelProminence_DP_Act = new QAction( tr("Degree Prestige"),	this);
    layoutLevelProminence_DP_Act -> setShortcut(Qt::CTRL + Qt::SHIFT+ Qt::Key_I);
    layoutLevelProminence_DP_Act ->setStatusTip(
                tr("Place nodes on horizontal levels of height "
                    "proportional to their Degree Prestige."));
    layoutLevelProminence_DP_Act->
            setWhatsThis(
                tr("Degree Prestige (DP) Levels Layout\n\n"
                    "Repositions all nodes on horizontal levels of height"
                    "proportional to their Degree Prestige score. "
                    "Nodes having higher DP are closer to the top."
                    ));
    connect(layoutLevelProminence_DP_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutLevelByProminenceIndex()));

    layoutLevelProminence_PRP_Act = new QAction( tr("PageRank Prestige"),	this);
    layoutLevelProminence_PRP_Act -> setEnabled(true);
    layoutLevelProminence_PRP_Act -> setShortcut(Qt::CTRL + Qt::SHIFT+ Qt::Key_K);
    layoutLevelProminence_PRP_Act -> setStatusTip(
                tr("Place nodes on horizontal levels of height "
                    "proportional to their PageRank Prestige."));
    layoutLevelProminence_PRP_Act->
            setWhatsThis(
                tr("PageRank Prestige (PRP) Levels Layout\n\n"
                    "Repositions all nodes on horizontal levels of height"
                    "proportional to their PageRank Prestige score. "
                    "Nodes having higher PRP are closer to the top."
                    ));
    connect(layoutLevelProminence_PRP_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutLevelByProminenceIndex()));


    layoutLevelProminence_PP_Act = new QAction( tr("Proximity Prestige"),	this);
    layoutLevelProminence_PP_Act -> setEnabled(true);
    layoutLevelProminence_PP_Act -> setShortcut(Qt::CTRL + Qt::SHIFT+ Qt::Key_Y);
    layoutLevelProminence_PP_Act -> setStatusTip(
                tr("Place nodes on horizontal levels of height "
                    "proportional to their Proximity Prestige."));
    layoutLevelProminence_PP_Act->
            setWhatsThis(
                tr("Proximity Prestige (PP) Levels Layout\n\n"
                    "Repositions all nodes on horizontal levels of height"
                    "proportional to their Proximity Prestige score. "
                    "Nodes having higher PP are closer to the top."
                    ));
    connect(layoutLevelProminence_PP_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutLevelByProminenceIndex()));




    layoutNodeSizeProminence_DC_Act = new QAction( tr("Degree Centrality"), this);
    layoutNodeSizeProminence_DC_Act -> setShortcut(Qt::ALT+ Qt::Key_1);
    layoutNodeSizeProminence_DC_Act
            ->setStatusTip(
                tr("Resize all nodes to be "
                   "proportional to their Degree Centrality."));
    layoutNodeSizeProminence_DC_Act->
             setWhatsThis(
                 tr(
                     "Degree Centrality (DC) Node Size Layout\n\n"
                     "Changes the size of all nodes to be "
                     "proportional to their DC (inDegree) score. \n\n"
                     "Nodes having higher DC will appear bigger."
                    )
                );
    connect(layoutNodeSizeProminence_DC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeSizeByProminenceIndex()) );

    layoutNodeSizeProminence_CC_Act = new QAction( tr("Closeness Centrality"), this);
    layoutNodeSizeProminence_CC_Act -> setShortcut(Qt::ALT+ Qt::Key_2);
    layoutNodeSizeProminence_CC_Act
            -> setStatusTip(
                tr("Resize all nodes to be "
                     "proportional to their Closeness Centrality."));
    layoutNodeSizeProminence_CC_Act->
             setWhatsThis(
                 tr("Closeness Centrality (CC) Node Size Layout\n\n"
                     "Changes the size of all nodes to be "
                     "proportional to their CC score. "
                     "Nodes of higher CC will appear bigger.\n\n"
                     "This layout can be computed only for connected graphs. "
                    ));
    connect(layoutNodeSizeProminence_CC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeSizeByProminenceIndex()));


    layoutNodeSizeProminence_IRCC_Act = new QAction(
                tr("Influence Range Closeness Centrality"),	this);
    layoutNodeSizeProminence_IRCC_Act -> setShortcut(Qt::ALT+ Qt::Key_3);
    layoutNodeSizeProminence_IRCC_Act
            ->setStatusTip(
                tr("Resize all nodes to be proportional "
                    "to their Influence Range Closeness Centrality."));
    layoutNodeSizeProminence_IRCC_Act->
             setWhatsThis(
                 tr("Influence Range Closeness Centrality (IRCC) Node Size Layout\n\n"
                     "Changes the size of all nodes to be "
                     "proportional to their IRCC score. "
                     "Nodes having higher IRCC will appear bigger.\n\n"
                     "This layout can be computed for not connected graphs. "
                    ));
    connect(layoutNodeSizeProminence_IRCC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeSizeByProminenceIndex()));

    layoutNodeSizeProminence_BC_Act = new QAction( tr("Betweenness Centrality"), this);
    layoutNodeSizeProminence_BC_Act -> setShortcut(Qt::ALT+ Qt::Key_4);
    layoutNodeSizeProminence_BC_Act ->setStatusTip(
                tr("Resize all nodes to be "
                     "proportional to their Betweenness Centrality."));
    layoutNodeSizeProminence_BC_Act->
             setWhatsThis(
                 tr("Betweenness Centrality (BC) Node Size Layout\n\n"
                     "Changes the size of all nodes to be "
                     "proportional to their Betweenness Centrality score. "
                     "Nodes having higher BC will appear bigger."
                    ));
    connect(layoutNodeSizeProminence_BC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeSizeByProminenceIndex()));

    layoutNodeSizeProminence_SC_Act = new QAction( tr("Stress Centrality"),	this);
    layoutNodeSizeProminence_SC_Act -> setShortcut(Qt::ALT+ Qt::Key_5);
    layoutNodeSizeProminence_SC_Act ->setStatusTip(
                tr( "Resize all nodes to be  "
                     "proportional to their Stress Centrality."));
    layoutNodeSizeProminence_SC_Act->
             setWhatsThis(
                 tr("Stress Centrality (SC) Node Size Layout\n\n"
                     "Changes the size of all nodes to be "
                     "proportional to their Stress Centrality score. "
                     "Nodes having higher SC will appear bigger."
                    ));
    connect(layoutNodeSizeProminence_SC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeSizeByProminenceIndex()));

    layoutNodeSizeProminence_EC_Act = new QAction( tr("Eccentricity Centrality"),	this);
    layoutNodeSizeProminence_EC_Act -> setShortcut(Qt::ALT+ Qt::Key_6);
    layoutNodeSizeProminence_EC_Act ->setStatusTip(
                tr("Resize all nodes to be "
                     "proportional to their Eccentricity Centrality."));
    layoutNodeSizeProminence_EC_Act->
             setWhatsThis(
                 tr("Eccentricity Centrality (EC) NodeSizes Layout\n\n"
                     "Changes the size of all nodes to be "
                     "proportional to their Eccentricity Centrality score. "
                     "Nodes having higher EC will appear bigger."
                    ));
    connect(layoutNodeSizeProminence_EC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeSizeByProminenceIndex()));


    layoutNodeSizeProminence_PC_Act = new QAction( tr("Power Centrality"),	this);
    layoutNodeSizeProminence_PC_Act -> setShortcut(Qt::ALT+ Qt::Key_7);
    layoutNodeSizeProminence_PC_Act ->setStatusTip(
                tr("Resize all nodes to be "
                     "proportional to their Power Centrality."));
    layoutNodeSizeProminence_PC_Act->
             setWhatsThis(
                 tr("Power Centrality (PC) Node Size Layout\n\n"
                    "Changes the size of all nodes to be "
                     "proportional to their Power Centrality score. "
                     "Nodes having higher PC will appear bigger."
                    ));
    connect(layoutNodeSizeProminence_PC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeSizeByProminenceIndex()));


    layoutNodeSizeProminence_IC_Act = new QAction( tr("Information Centrality"),	this);
    layoutNodeSizeProminence_IC_Act ->setEnabled(true);
    layoutNodeSizeProminence_IC_Act -> setShortcut(Qt::ALT+ Qt::Key_8);
    layoutNodeSizeProminence_IC_Act ->setStatusTip(
                tr("Resize all nodes to be "
                     "proportional to their Information Centrality."));
    layoutNodeSizeProminence_IC_Act->
             setWhatsThis(
                 tr("Information Centrality (IC) Node Size Layout\n\n"
                     "Changes the size of all nodes to be "
                     "proportional to their Information Centrality score. "
                     "Nodes having higher IC will appear bigger."
                    ));
    connect(layoutNodeSizeProminence_IC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeSizeByProminenceIndex()));

    layoutNodeSizeProminence_EVC_Act = new QAction( tr("Eigenvector Centrality"),	this);
    layoutNodeSizeProminence_EVC_Act ->setEnabled(true);
    layoutNodeSizeProminence_EVC_Act -> setShortcut(Qt::ALT+ Qt::Key_9);
    layoutNodeSizeProminence_EVC_Act ->setStatusTip(
                tr("Resize all nodes to be "
                     "proportional to their Eigenvector Centrality."));
    layoutNodeSizeProminence_EVC_Act->
             setWhatsThis(
                 tr("Eigenvector Centrality (EVC) Node Size Layout\n\n"
                     "Changes the size of all nodes to be "
                     "proportional to their Eigenvector Centrality score. "
                     "Nodes having higher EVC will appear bigger."
                    ));
    connect(layoutNodeSizeProminence_EVC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeSizeByProminenceIndex()));



    layoutNodeSizeProminence_DP_Act = new QAction( tr("Degree Prestige"),	this);
    layoutNodeSizeProminence_DP_Act -> setShortcut(Qt::ALT + Qt::Key_I);
    layoutNodeSizeProminence_DP_Act ->setStatusTip(
                tr("Resize all nodes to be "
                    "proportional to their Degree Prestige."));
    layoutNodeSizeProminence_DP_Act->
            setWhatsThis(
                tr("Degree Prestige (DP) Node Size Layout\n\n"
                    "Changes the size of all nodes to be "
                    "proportional to their Degree Prestige score. "
                    "Nodes having higher DP will appear bigger."
                    ));
    connect(layoutNodeSizeProminence_DP_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeSizeByProminenceIndex()));

    layoutNodeSizeProminence_PRP_Act = new QAction( tr("PageRank Prestige"),	this);
    layoutNodeSizeProminence_PRP_Act -> setEnabled(true);
    layoutNodeSizeProminence_PRP_Act -> setShortcut(Qt::ALT+ Qt::Key_K);
    layoutNodeSizeProminence_PRP_Act -> setStatusTip(
                tr("Resize all nodes to be "
                    "proportional to their PageRank Prestige."));
    layoutNodeSizeProminence_PRP_Act->
            setWhatsThis(
                tr("PageRank Prestige (PRP) Node Size Layout\n\n"
                    "Changes the size of all nodes to be "
                    "proportional to their PageRank Prestige score. "
                    "Nodes having higher PRP will appear bigger."
                    ));
    connect(layoutNodeSizeProminence_PRP_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeSizeByProminenceIndex()));


    layoutNodeSizeProminence_PP_Act = new QAction( tr("Proximity Prestige"),	this);
    layoutNodeSizeProminence_PP_Act -> setEnabled(true);
    layoutNodeSizeProminence_PP_Act -> setShortcut(
                QKeySequence(Qt::CTRL + Qt::Key_L, Qt::CTRL + Qt::Key_S, Qt::CTRL + Qt::Key_R)
                //Qt::ALT + Qt::Key_Y
                );
    layoutNodeSizeProminence_PP_Act -> setStatusTip(
                tr("Resize all nodes to be "
                    "proportional to their Proximity Prestige."));
    layoutNodeSizeProminence_PP_Act->
            setWhatsThis(
                tr("Proximity Prestige (PP) Node Size Layout\n\n"
                   "Changes the size of all nodes to be "
                   "proportional to their Proximity Prestige score. "
                   "Nodes having higher PP will appear bigger."
                    ));
    connect(layoutNodeSizeProminence_PP_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeSizeByProminenceIndex()));





    layoutNodeColorProminence_DC_Act = new QAction( tr("Degree Centrality"), this);
    layoutNodeColorProminence_DC_Act -> setShortcut(
                QKeySequence(Qt::CTRL + Qt::Key_L, Qt::CTRL + Qt::Key_C, Qt::CTRL + Qt::Key_1)
                );
    layoutNodeColorProminence_DC_Act
            ->setStatusTip(
                tr("Change the color of all nodes to "
                   "reflect their Degree Centrality."));
    layoutNodeColorProminence_DC_Act->
             setWhatsThis(
                 tr("Degree Centrality (DC) Node Color Layout\n\n"
                     "Changes the color of all nodes to "
                     "reflect their DC (inDegree) score. \n\n"
                     "Nodes having higher DC will have warmer color (i.e. red)."
                    )
                );
    connect(layoutNodeColorProminence_DC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeColorByProminenceIndex()) );

    layoutNodeColorProminence_CC_Act = new QAction( tr("Closeness Centrality"), this);
    layoutNodeColorProminence_CC_Act -> setShortcut(
                QKeySequence(Qt::CTRL + Qt::Key_L, Qt::CTRL + Qt::Key_C, Qt::CTRL + Qt::Key_2)
                );
    layoutNodeColorProminence_CC_Act
            -> setStatusTip(
                tr("Change the color of all nodes to "
                     "reflect their Closeness Centrality."));
    layoutNodeColorProminence_CC_Act->
             setWhatsThis(
                 tr("Closeness Centrality (CC) Node Color Layout\n\n"
                     "Changes the color of all nodes to "
                     "reflect their CC score. "
                     "Nodes of higher CC will have warmer color (i.e. red).\n\n"
                     "This layout can be computed only for connected graphs. "
                    ));
    connect(layoutNodeColorProminence_CC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeColorByProminenceIndex()));


    layoutNodeColorProminence_IRCC_Act = new QAction(
                tr("Influence Range Closeness Centrality"),	this);
    layoutNodeColorProminence_IRCC_Act -> setShortcut(
                QKeySequence(Qt::CTRL + Qt::Key_L, Qt::CTRL + Qt::Key_C, Qt::CTRL + Qt::Key_3)
                );
    layoutNodeColorProminence_IRCC_Act
            ->setStatusTip(
                tr("Change the color of all nodes to proportional "
                    "to their Influence Range Closeness Centrality."));
    layoutNodeColorProminence_IRCC_Act->
             setWhatsThis(
                 tr("Influence Range Closeness Centrality (IRCC) Node Color Layout\n\n"
                     "Changes the color of all nodes to "
                     "reflect their IRCC score. "
                     "Nodes having higher IRCC will have warmer color (i.e. red).\n\n"
                     "This layout can be computed for not connected graphs. "
                    ));
    connect(layoutNodeColorProminence_IRCC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeColorByProminenceIndex()));

    layoutNodeColorProminence_BC_Act = new QAction( tr("Betweenness Centrality"), this);
    layoutNodeColorProminence_BC_Act -> setShortcut(
                QKeySequence(Qt::CTRL + Qt::Key_L, Qt::CTRL + Qt::Key_C, Qt::CTRL + Qt::Key_4)
                );
    layoutNodeColorProminence_BC_Act ->setStatusTip(
                tr("Change the color of all nodes to "
                     "reflect their Betweenness Centrality."));
    layoutNodeColorProminence_BC_Act->
             setWhatsThis(
                 tr("Betweenness Centrality (BC) Node Color Layout\n\n"
                     "Changes the color of all nodes to "
                     "reflect their Betweenness Centrality score. "
                     "Nodes having higher BC will have warmer color (i.e. red)."
                    ));
    connect(layoutNodeColorProminence_BC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeColorByProminenceIndex()));

    layoutNodeColorProminence_SC_Act = new QAction( tr("Stress Centrality"),	this);
    layoutNodeColorProminence_SC_Act -> setShortcut(
                QKeySequence(Qt::CTRL + Qt::Key_L, Qt::CTRL + Qt::Key_C, Qt::CTRL + Qt::Key_5)
                );
    layoutNodeColorProminence_SC_Act ->setStatusTip(
                tr( "Change the color of all nodes to  "
                     "reflect their Stress Centrality."));
    layoutNodeColorProminence_SC_Act->
             setWhatsThis(
                 tr("Stress Centrality (SC) Node Color Layout\n\n"
                     "Changes the color of all nodes to "
                     "reflect their Stress Centrality score. "
                     "Nodes having higher SC will have warmer color (i.e. red)."
                    ));
    connect(layoutNodeColorProminence_SC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeColorByProminenceIndex()));

    layoutNodeColorProminence_EC_Act = new QAction( tr("Eccentricity Centrality"),	this);
    layoutNodeColorProminence_EC_Act -> setShortcut(
                QKeySequence(Qt::CTRL + Qt::Key_L, Qt::CTRL + Qt::Key_C, Qt::CTRL + Qt::Key_6)
                );
    layoutNodeColorProminence_EC_Act ->setStatusTip(
                tr("Change the color of all nodes to "
                     "reflect their Eccentricity Centrality."));
    layoutNodeColorProminence_EC_Act->
             setWhatsThis(
                 tr("Eccentricity Centrality (EC) NodeColors Layout\n\n"
                     "Changes the color of all nodes to "
                     "reflect their Eccentricity Centrality score. "
                     "Nodes having higher EC will have warmer color (i.e. red)."
                    ));
    connect(layoutNodeColorProminence_EC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeColorByProminenceIndex()));


    layoutNodeColorProminence_PC_Act = new QAction( tr("Power Centrality"),	this);
    layoutNodeColorProminence_PC_Act -> setShortcut(
                QKeySequence(Qt::CTRL + Qt::Key_L, Qt::CTRL + Qt::Key_C, Qt::CTRL + Qt::Key_7)
                );
    layoutNodeColorProminence_PC_Act ->setStatusTip(
                tr("Change the color of all nodes to "
                     "reflect their Power Centrality."));
    layoutNodeColorProminence_PC_Act->
             setWhatsThis(
                 tr("Power Centrality (PC) Node Color Layout\n\n"
                    "Changes the color of all nodes to "
                     "reflect their Power Centrality score. "
                     "Nodes having higher PC will have warmer color (i.e. red)."
                    ));
    connect(layoutNodeColorProminence_PC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeColorByProminenceIndex()));


    layoutNodeColorProminence_IC_Act = new QAction( tr("Information Centrality"),	this);
    layoutNodeColorProminence_IC_Act ->setEnabled(true);
    layoutNodeColorProminence_IC_Act -> setShortcut(
                QKeySequence(Qt::CTRL + Qt::Key_L, Qt::CTRL + Qt::Key_C, Qt::CTRL + Qt::Key_8)
                );
    layoutNodeColorProminence_IC_Act ->setStatusTip(
                tr("Change the color of all nodes to "
                     "reflect their Information Centrality."));
    layoutNodeColorProminence_IC_Act->
             setWhatsThis(
                 tr("Information Centrality (IC) Node Color Layout\n\n"
                     "Changes the color of all nodes to "
                     "reflect their Information Centrality score. "
                     "Nodes having higher IC will have warmer color (i.e. red)."
                    ));
    connect(layoutNodeColorProminence_IC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeColorByProminenceIndex()));

    layoutNodeColorProminence_EVC_Act = new QAction( tr("Eigenvector Centrality"),	this);
    layoutNodeColorProminence_EVC_Act ->setEnabled(true);
    layoutNodeColorProminence_EVC_Act -> setShortcut(
                QKeySequence(Qt::CTRL + Qt::Key_L, Qt::CTRL + Qt::Key_C, Qt::CTRL + Qt::Key_9)
                );
    layoutNodeColorProminence_EVC_Act ->setStatusTip(
                tr("Change the color of all nodes to "
                     "reflect their Eigenvector Centrality."));
    layoutNodeColorProminence_EVC_Act->
             setWhatsThis(
                 tr("Eigenvector Centrality (EVC) Node Color Layout\n\n"
                     "Changes the color of all nodes to "
                     "reflect their Eigenvector Centrality score. "
                     "Nodes having higher EVC will have warmer color (i.e. red)."
                    ));
    connect(layoutNodeColorProminence_EVC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeColorByProminenceIndex()));



    layoutNodeColorProminence_DP_Act = new QAction( tr("Degree Prestige"),	this);
    layoutNodeColorProminence_DP_Act -> setShortcut(
                QKeySequence(Qt::CTRL + Qt::Key_L, Qt::CTRL + Qt::Key_C, Qt::CTRL + Qt::Key_D)
                                                    );
    layoutNodeColorProminence_DP_Act ->setStatusTip(
                tr("Change the color of all nodes to "
                    "reflect their Degree Prestige."));
    layoutNodeColorProminence_DP_Act->
            setWhatsThis(
                tr("Degree Prestige (DP) Node Color Layout\n\n"
                    "Changes the color of all nodes to "
                    "reflect their Degree Prestige score. "
                    "Nodes having higher DP will have warmer color (i.e. red)."
                    ));
    connect(layoutNodeColorProminence_DP_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeColorByProminenceIndex()));

    layoutNodeColorProminence_PRP_Act = new QAction( tr("PageRank Prestige"),	this);
    layoutNodeColorProminence_PRP_Act -> setEnabled(true);
    layoutNodeColorProminence_PRP_Act -> setShortcut(
                QKeySequence(Qt::CTRL + Qt::Key_L, Qt::CTRL + Qt::Key_C, Qt::CTRL + Qt::Key_R)
                );
    layoutNodeColorProminence_PRP_Act -> setStatusTip(
                tr("Change the color of all nodes to "
                    "reflect their PageRank Prestige."));
    layoutNodeColorProminence_PRP_Act->
            setWhatsThis(
                tr("PageRank Prestige (PRP) Node Color Layout\n\n"
                    "Changes the color of all nodes to "
                    "reflect their PageRank Prestige score. "
                    "Nodes having higher PRP will have warmer color (i.e. red)."
                    ));
    connect(layoutNodeColorProminence_PRP_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeColorByProminenceIndex()));


    layoutNodeColorProminence_PP_Act = new QAction( tr("Proximity Prestige"),	this);
    layoutNodeColorProminence_PP_Act -> setEnabled(true);
    layoutNodeColorProminence_PP_Act -> setShortcut(
                QKeySequence(Qt::CTRL + Qt::Key_L, Qt::CTRL + Qt::Key_C, Qt::CTRL + Qt::Key_P)
                );
    layoutNodeColorProminence_PP_Act -> setStatusTip(
                tr("Change the color of all nodes to "
                    "reflect their Proximity Prestige."));
    layoutNodeColorProminence_PP_Act->
            setWhatsThis(
                tr("Proximity Prestige (PP) Node Color Layout\n\n"
                   "Changes the color of all nodes to "
                   "reflect their PageRank Prestige score. "
                   "Nodes of higher PP will have warmer color (i.e. red)."
                   ));
    connect(layoutNodeColorProminence_PP_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeColorByProminenceIndex()));





    layoutFDP_Eades_Act= new QAction(tr("Spring Embedder (Eades)"), this);
    layoutFDP_Eades_Act-> setShortcut(
                QKeySequence(Qt::CTRL + Qt::Key_L, Qt::CTRL + Qt::Key_E));
    layoutFDP_Eades_Act->setStatusTip(
                tr("Layout Eades Spring-Gravitational model."));
    layoutFDP_Eades_Act->setWhatsThis(
                tr("Spring Embedder Layout\n\n "
                   "The Spring Embedder model (Eades, 1984), part of the "
                   "Force Directed Placement (FDP) family, embeds a mechanical "
                   "system in the graph by replacing nodes with rings and edges "
                   "with springs. \n"
                   "In our implementation, nodes are replaced by physical bodies "
                   "(i.e. electrons) which exert repelling forces to each other, "
                   "while edges are replaced by springs which exert attractive "
                   "forces to the adjacent nodes. "
                   "The nodes are placed in some initial layout and let go "
                   "so that the spring forces move the system to a minimal energy state. "
                   "The algorithm continues until the system retains an equilibrium state "
                   "in which all forces cancel each other. "));
    connect(layoutFDP_Eades_Act, SIGNAL(triggered(bool)), this, SLOT(slotLayoutSpringEmbedder()));

    layoutFDP_FR_Act= new QAction( tr("Fruchterman-Reingold"),	this);
    layoutFDP_FR_Act-> setShortcut(
                QKeySequence(Qt::CTRL + Qt::Key_L, Qt::CTRL + Qt::Key_F));
    layoutFDP_FR_Act->setStatusTip(
                tr("Repelling forces between all nodes, and attracting forces between adjacent nodes."));
    layoutFDP_FR_Act->setWhatsThis(
                tr("Fruchterman-Reingold Layout\n\n "
                   "Embeds a layout all nodes according to a model in which	repelling "
                   "forces are used between every pair of nodes, while attracting "
                   "forces are used only between adjacent nodes. "
                   "The algorithm continues until the system retains its equilibrium "
                   "state where all forces cancel each other."));
    connect(layoutFDP_FR_Act, SIGNAL(triggered()), this, SLOT(slotLayoutFruchterman()));

    layoutFDP_KamadaKawai_Act= new QAction( tr("Kamada-Kawai"),	this);
    layoutFDP_KamadaKawai_Act-> setShortcut(
                QKeySequence(Qt::CTRL + Qt::Key_L, Qt::CTRL + Qt::Key_K));
    layoutFDP_KamadaKawai_Act->setStatusTip(
                tr("Repelling forces between all nodes, and attracting forces between adjacent nodes."));
    layoutFDP_KamadaKawai_Act->setWhatsThis(
                tr("Fruchterman-Reingold Layout\n\n "
                   "Embeds a layout all nodes according to a model in which	repelling "
                   "forces are used between every pair of nodes, while attracting "
                   "forces are used only between adjacent nodes. "
                   "The algorithm continues until the system retains its equilibrium state where "
                   "all forces cancel each other."));
    connect(layoutFDP_KamadaKawai_Act, SIGNAL(triggered()), this, SLOT(slotLayoutKamadaKawai()));




    layoutGuidesAct = new QAction(QIcon(":/images/gridlines.png"), tr("Layout GuideLines"), this);
    layoutGuidesAct ->setStatusTip(tr("Toggles layout guidelines on or off."));
    layoutGuidesAct->setWhatsThis(tr("Layout Guidelines\n\n"
                                     "Layout Guidelines are circular or horizontal lines \n"
                                     "usually created when embedding prominence-based \n"
                                     "visualization models on the network.\n"
                                     "Disable this checkbox to hide guidelines"));
    layoutGuidesAct->setCheckable(true);
    layoutGuidesAct->setChecked(true);


    /**
    Analysis menu actions
    */


    analyzeMatrixAdjInvertAct = new QAction(
                QIcon(":/images/invertmatrix.png"), tr("Invert Adjacency Matrix"), this);
    analyzeMatrixAdjInvertAct -> setShortcut(
                QKeySequence(Qt::CTRL + Qt::Key_M, Qt::CTRL + Qt::Key_I)
                );
    analyzeMatrixAdjInvertAct->setStatusTip(tr("Invert the adjacency matrix, if possible"));
    analyzeMatrixAdjInvertAct->setWhatsThis(tr("Invert  Adjacency Matrix \n\n"
                                               "Inverts the adjacency matrix using linear algebra methods."));
    connect(analyzeMatrixAdjInvertAct, SIGNAL(triggered()),
            this, SLOT(slotAnalyzeMatrixAdjacencyInverse()));


    analyzeMatrixAdjTransposeAct = new QAction(
                QIcon(":/images/transposematrix.png"), tr("Transpose Adjacency Matrix"), this);
    analyzeMatrixAdjTransposeAct -> setShortcut(
                QKeySequence(Qt::CTRL + Qt::Key_M, Qt::CTRL + Qt::Key_T)
                );
    analyzeMatrixAdjTransposeAct->setStatusTip(tr("View the transpose of adjacency matrix"));
    analyzeMatrixAdjTransposeAct->setWhatsThis(tr("Transpose Adjacency Matrix \n\n"
                                                  "Computes and displays the adjacency matrix tranpose."));
    connect(analyzeMatrixAdjTransposeAct, SIGNAL(triggered()),
            this, SLOT(slotAnalyzeMatrixAdjacencyTranspose()));


    analyzeMatrixAdjCocitationAct = new QAction(
                QIcon(":/images/cocitation.png"), tr("Cocitation Matrix"), this);
    analyzeMatrixAdjCocitationAct -> setShortcut(
                QKeySequence(Qt::CTRL + Qt::Key_M, Qt::CTRL + Qt::Key_C)
                );
    analyzeMatrixAdjCocitationAct->setStatusTip(tr("Compute the Cocitation matrix of this network."));
    analyzeMatrixAdjCocitationAct->setWhatsThis(tr("Cocitation Matrix \n\n "
                                                   "Computes and displays the cocitation matrix of the network. "
                                                   "The Cocitation matrix, C=A*A^T, is a NxN matrix where "
                                                   "each element (i,j) is the number of actors that have "
                                                   "outbound ties/links to both actors i and j. "));
    connect(analyzeMatrixAdjCocitationAct, SIGNAL(triggered()),
            this, SLOT(slotAnalyzeMatrixAdjacencyCocitation()));


    analyzeMatrixDegreeAct = new QAction(
                QIcon(":/images/degreematrix.png"), tr("Degree Matrix"), this);
    analyzeMatrixDegreeAct -> setShortcut(
                QKeySequence(Qt::CTRL + Qt::Key_M, Qt::CTRL + Qt::Key_D)
                );
    analyzeMatrixDegreeAct->setStatusTip(tr("Compute the Degree matrix of the network"));
    analyzeMatrixDegreeAct->setWhatsThis(tr("Degree Matrix "
                                        "\n\n Compute the Degree matrix of the network."));
    connect(analyzeMatrixDegreeAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeMatrixDegree()));


    analyzeMatrixLaplacianAct = new QAction(
                QIcon(":/images/laplacian.png"), tr("Laplacian Matrix"), this);
    analyzeMatrixLaplacianAct -> setShortcut(
                QKeySequence(Qt::CTRL + Qt::Key_M, Qt::CTRL + Qt::Key_L)
                );
    analyzeMatrixLaplacianAct->setStatusTip(tr("Compute the Laplacian matrix of the network"));
    analyzeMatrixLaplacianAct->setWhatsThis(tr("Laplacian Matrix \n\n"
                                               "Compute the Laplacian matrix of the network."));
    connect(analyzeMatrixLaplacianAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeMatrixLaplacian()));



    analyzeGraphReciprocityAct = new QAction(
                QIcon(":/images/symmetry-edge.png"), tr("Reciprocity"), this);
    analyzeGraphReciprocityAct -> setShortcut(
                QKeySequence(Qt::CTRL + Qt::Key_G, Qt::CTRL + Qt::Key_R)
                );
    analyzeGraphReciprocityAct->setStatusTip(tr("Compute the arc and dyad reciprocity of the network."));
    analyzeGraphReciprocityAct->setWhatsThis(
                tr("Arc and Dyad Reciprocity\n\n"
                   "The arc reciprocity of a network/graph is the fraction of "
                   "reciprocated ties over all present ties of the graph. \n"
                   "The dyad reciprocity of a network/graph is the fraction of "
                   "actor pairs that have reciprocated ties over all connected "
                   "pairs of actors. \n"
                   "In a directed network, the arc reciprocity measures the proportion "
                   "of directed edges that are bidirectional. If the reciprocity is 1, \n"
                   "then the adjacency matrix is structurally symmetric. \n"
                   "Likewise, in a directed network, the dyad reciprocity measures "
                   "the proportion of connected actor dyads that have bidirectional ties "
                   "between them. \n"
                   "In an undirected graph, all edges are reciprocal. Thus the "
                   "reciprocity of the graph is always 1. \n"
                   "Reciprocity can be computed on undirected, directed, and weighted graphs."
                   )
                );
    connect(analyzeGraphReciprocityAct, SIGNAL(triggered()),
            this, SLOT(slotAnalyzeReciprocity()));

    analyzeGraphSymmetryAct = new QAction(
                QIcon(":/images/symmetry-edge.png"), tr("Symmetry Test"), this);
    analyzeGraphSymmetryAct -> setShortcut(
                QKeySequence(Qt::CTRL + Qt::Key_G, Qt::CTRL + Qt::Key_S)
                );
    analyzeGraphSymmetryAct->setStatusTip(tr("Check whether the network is symmetric or not"));
    analyzeGraphSymmetryAct->setWhatsThis(
                tr("Symmetry\n\n"
                   "Checks whether the network is symmetric or not. \n"
                   "A network is symmetric when all edges are reciprocal, or, "
                   "in mathematical language, when the adjacency matrix is "
                   "symmetric.")
                );
    connect(analyzeGraphSymmetryAct, SIGNAL(triggered()),
            this, SLOT(slotAnalyzeSymmetryCheck()));

    analyzeGraphDistanceAct = new QAction(
                QIcon(":/images/distance.png"), tr("Geodesic Distance between 2 nodes"), this
                );
    analyzeGraphDistanceAct -> setShortcut(
                QKeySequence(Qt::CTRL + Qt::Key_G, Qt::CTRL + Qt::Key_G) );
    analyzeGraphDistanceAct->setStatusTip(
                tr("Compute the length of the shortest path (geodesic distance) between 2 nodes."));
    analyzeGraphDistanceAct->setWhatsThis(
                tr("Distance\n\n"
                   "Computes the geodesic distance between two nodes."
                   "In graph theory, the geodesic distance of two "
                   "nodes is the length (number of edges) of the shortest path "
                   "between them."));
    connect(analyzeGraphDistanceAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeDistance()));


    analyzeMatrixDistancesGeodesicAct = new QAction(QIcon(":/images/dm.png"), tr("Geodesic Distances Matrix"),this);
    analyzeMatrixDistancesGeodesicAct -> setShortcut(
                QKeySequence(Qt::CTRL + Qt::Key_G, Qt::CTRL + Qt::Key_M) );
    analyzeMatrixDistancesGeodesicAct->
            setStatusTip(
                tr("Compute the matrix of geodesic distances between all pair of nodes.")
                );
    analyzeMatrixDistancesGeodesicAct->
            setWhatsThis(
                tr("Distances Matrix\n\n"
                   "Computes the matrix of distances between all "
                   "pairs of actors/nodes in the social network."
                   "A distances matrix is a n x n matrix, in which the "
                   "(i,j) element is the distance from node i to node j"
                   "The distance of two nodes is the length of the shortest path between them.")
                );
    connect(analyzeMatrixDistancesGeodesicAct, SIGNAL(triggered()), this, SLOT( slotAnalyzeMatrixDistances() ) );

    analyzeMatrixGeodesicsAct = new QAction(QIcon(":/images/dm.png"), tr("Geodesics Matrix"),this);
    analyzeMatrixGeodesicsAct -> setShortcut(
                QKeySequence(Qt::CTRL + Qt::Key_G, Qt::CTRL + Qt::Key_P));
    analyzeMatrixGeodesicsAct->setStatusTip(tr("Compute the number of shortest paths (geodesics) between each pair of nodes "));
    analyzeMatrixGeodesicsAct->setWhatsThis(
                tr(
                    "Geodesics Matrix\n\n"
                    "Displays a n x n matrix, where the (i,j) element "
                    "is the number of shortest paths (geodesics) between "
                    "node i and node j. ")
                );
    connect(analyzeMatrixGeodesicsAct, SIGNAL(triggered()),
            this, SLOT( slotAnalyzeMatrixGeodesics()) );

    analyzeGraphDiameterAct = new QAction(QIcon(":/images/diameter.png"), tr("Graph Diameter"),this);
    analyzeGraphDiameterAct -> setShortcut(
                QKeySequence(Qt::CTRL + Qt::Key_G, Qt::CTRL + Qt::Key_D));
    analyzeGraphDiameterAct->setStatusTip(tr("Compute the diameter of the network, "
                                 "the maximum geodesic distance between any actors."));
    analyzeGraphDiameterAct->setWhatsThis(tr("Diameter\n\n "
                                 "The Diameter of a social network is the maximum geodesic distance "
                                 "(maximum shortest path length) between any two nodes of the network."));
    connect(analyzeGraphDiameterAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeDiameter()));

    averGraphDistanceAct = new QAction(QIcon(":/images/avdistance.png"), tr("Average Distance"),this);
    averGraphDistanceAct -> setShortcut(
                QKeySequence(Qt::CTRL + Qt::Key_G, Qt::CTRL + Qt::Key_A));
    averGraphDistanceAct->setStatusTip(tr("Compute the average length of shortest paths for all possible pairs of nodes."));
    averGraphDistanceAct->setWhatsThis(
                tr("Average Distance\n\n "
                   "Computes the average length of shortest paths (geodesics) "
                   "between all pairs of network actors (vertices in the graph). "
                   "It is a measure of the efficiency or compactness of the network."));
    connect(averGraphDistanceAct, SIGNAL(triggered()),
            this, SLOT(slotAnalyzeDistanceAverage()));

    analyzeGraphEccentricityAct = new QAction(QIcon(":/images/eccentricity.png"), tr("Eccentricity"),this);
    analyzeGraphEccentricityAct-> setShortcut(
                QKeySequence(Qt::CTRL + Qt::Key_G, Qt::CTRL + Qt::Key_E ) );
    analyzeGraphEccentricityAct->setStatusTip(tr("Compute the Eccentricity of each actor and group Eccentricity"));
    analyzeGraphEccentricityAct->setWhatsThis(tr("Eccentricity\n\n"
                                     "The eccentricity of each node i in a network "
                                     "or graph is the largest geodesic distance "
                                     "between node i and any other node j. "
                                     "Therefore, it reflects how far, at most, "
                                     "is each node from every other node. \n"
                                     "The maximum eccentricity is the graph diameter "
                                     "while the minimum is the graph radius.\n"
                                     "This index can be calculated in both graphs "
                                     "and digraphs but is usually best suited "
                                     "for undirected graphs. \n"
                                     "It can also be calculated in weighted graphs "
                                     "although the weight of each edge (v,u) in E is "
                                     "always considered to be 1."));
    connect(analyzeGraphEccentricityAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeEccentricity()));



    analyzeGraphConnectednessAct = new QAction(QIcon(":/images/distance.png"),  tr("Connectedness"), this);
    analyzeGraphConnectednessAct -> setShortcut(
                QKeySequence(Qt::CTRL + Qt::Key_G, Qt::CTRL + Qt::Key_C) );
    analyzeGraphConnectednessAct->setStatusTip(tr("Check whether the network is a connected "
                                      "graph, a weakly connected digraph or "
                                      "a disconnected graph/digraph..."));
    analyzeGraphConnectednessAct->setWhatsThis(tr("Connectedness\n\n In graph theory, a "
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
    connect(analyzeGraphConnectednessAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeConnectedness()));


    analyzeGraphWalksAct = new QAction(QIcon(":/images/walk.png"), tr("Walks of a given length"),this);
    analyzeGraphWalksAct-> setShortcut(
                QKeySequence(Qt::CTRL + Qt::Key_G, Qt::CTRL + Qt::Key_W) );
    analyzeGraphWalksAct->setStatusTip(tr("Compute the number of walks of a given length between any nodes."));
    analyzeGraphWalksAct->setWhatsThis(tr("Walks of a given length\n\n"
                              "A walk is a sequence of alternating vertices and edges "
                              "such as v<sub>0</sub>e<sub>1</sub>, v<sub>1</sub>e<sub>2</sub>, "
                              "v<sub>2</sub>e<sub>3</sub>, …, e<sub>k</sub>v<sub>k</sub>, "
                              "where each edge, e<sub>i</sub> is defined as "
                              "e<sub>i</sub> = {v<sub>i-1</sub>, v<sub>i</sub>}. "
                              "This function counts the number of walks of a given "
                              "length between each pair of nodes, by studying the powers of the sociomatrix.\n"));
    connect(analyzeGraphWalksAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeWalksLength() )  );

    analyzeGraphWalksTotalAct = new QAction(QIcon(":/images/walk.png"), tr("Total Walks"),this);
    analyzeGraphWalksTotalAct-> setShortcut(
                QKeySequence(Qt::CTRL + Qt::Key_G, Qt::CTRL + Qt::Key_T) );
    analyzeGraphWalksTotalAct->setStatusTip(tr("Calculate the total number of walks of every possible length between all nodes"));
    analyzeGraphWalksTotalAct->setWhatsThis(tr("Total Walks\n\n"
                                   "A walk is a sequence of alternating vertices "
                                   "and edges such as v<sub>0</sub>e<sub>1</sub>, "
                                   "v<sub>1</sub>e<sub>2</sub>, v<sub>2</sub>e<sub>3</sub>, …, "
                                   "e<sub>k</sub>v<sub>k</sub>, where each edge, e<sub>i</sub> "
                                   "is defined as e<sub>i</sub> = {v<sub>i-1</sub>, v<sub>i</sub>}. "
                                   "This function counts the number of walks of any length "
                                   "between each pair of nodes, by studying the powers of the sociomatrix. \n"));
    connect(analyzeGraphWalksTotalAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeWalksTotal() )  );


    analyzeMatrixReachabilityAct = new QAction(QIcon(":/images/walk.png"), tr("Reachability Matrix"),this);
    analyzeMatrixReachabilityAct-> setShortcut(
                QKeySequence(Qt::CTRL + Qt::Key_M, Qt::CTRL + Qt::Key_R));
    analyzeMatrixReachabilityAct->setStatusTip(tr("Compute the Reachability Matrix of the network."));
    analyzeMatrixReachabilityAct->setWhatsThis(tr("Reachability Matrix\n\n"
                                           "Calculates the reachability matrix X<sup>R</sup> of "
                                           "the graph where the {i,j} element is 1 if "
                                           "the vertices i and j are reachable. \n\n"
                                           "Actually, this just checks whether the corresponding element "
                                           "of Distances matrix is not zero.\n"));
    connect(analyzeMatrixReachabilityAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeReachabilityMatrix() )  );



    clusteringCoefAct = new QAction(QIcon(":/images/clucof.png"), tr("Local and Network Clustering Coefficient"),this);
    clusteringCoefAct -> setShortcut(
                QKeySequence(Qt::CTRL + Qt::Key_G, Qt::CTRL + Qt::Key_L) );
    clusteringCoefAct->setStatusTip(tr("Compute the Watts & Strogatz Clustering Coefficient for every actor and the network average."));
    clusteringCoefAct->setWhatsThis(tr("Local and Network Clustering Coefficient\n\n"
                                       "The local Clustering Coefficient  (Watts & Strogatz, 1998) "
                                       "of an actor quantifies how close "
                                       "the actor and her neighbors are to being a clique and "
                                       "can be used as an indication of network transitivity. \n"));
    connect(clusteringCoefAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeClusteringCoefficient() )  );





    analyzeCommunitiesCliquesAct = new QAction(QIcon(":/images/clique.png"), tr("Clique Census"),this);
    analyzeCommunitiesCliquesAct-> setShortcut(
                QKeySequence(Qt::CTRL + Qt::Key_U, Qt::CTRL + Qt::Key_C));
    analyzeCommunitiesCliquesAct->setStatusTip(tr("Compute the clique census: find all maximal connected subgraphs."));
    analyzeCommunitiesCliquesAct->setWhatsThis(tr("Clique Census\n\n"
                                "Produces the census of network cliques (maximal connected subgraphs), "
                                "along with disaggregation by actor and co-membership information. "));
    connect(analyzeCommunitiesCliquesAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeCommunitiesCliqueCensus() )  );



    analyzeCommunitiesTriadCensusAct = new QAction(QIcon(":/images/triad.png"), tr("Triad Census (M-A-N labeling)"),this);
    analyzeCommunitiesTriadCensusAct-> setShortcut(
                QKeySequence(Qt::CTRL + Qt::Key_U, Qt::CTRL + Qt::Key_T) );
    analyzeCommunitiesTriadCensusAct->setStatusTip(tr("Calculate the triad census for all actors."));
    analyzeCommunitiesTriadCensusAct->setWhatsThis(tr("Triad Census\n\n"
                                    "A triad census counts all the different kinds of observed triads "
                                    "within a network and codes them according to their number of mutual, "
                                    "asymmetric and non-existent dyads using the M-A-N labeling scheme. \n"));
    connect(analyzeCommunitiesTriadCensusAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeCommunitiesTriadCensus() )  );



    analyzeStrEquivalencePearsonAct = new QAction(QIcon(":/images/similarity.png"),
                                                  tr("Pearson correlation coefficients"),this);
    analyzeStrEquivalencePearsonAct-> setShortcut(
                QKeySequence(Qt::CTRL + Qt::Key_T, Qt::CTRL + Qt::Key_P)
                );
    analyzeStrEquivalencePearsonAct->setStatusTip(
                tr("Compute Pearson Correlation Coefficients between pairs of actors. "
                   "Most useful with valued/weighted ties (non-binary). "));
    analyzeStrEquivalencePearsonAct->setWhatsThis(
                tr("Pearson correlation coefficients\n\n"
                   "Computes a correlation matrix, where the elements are the "
                   "Pearson correlation coefficients between pairs of actors "
                   "in terms of their tie profiles or distances (in, out or both). \n\n"
                   "The Pearson product-moment correlation coefficient (PPMCC or PCC or Pearson's r)"
                   "is a measure of the linear dependence/association between two variables X and Y. \n\n"
                   "This correlation measure of similarity is particularly useful "
                   "when ties are valued/weighted denoting strength, cost or probability.\n\n"
                   "Note that in very sparse networks (very low density), measures such as"
                   "\"exact matches\", \"correlation\" and \"distance\" "
                   "will show little variation among the actors, causing "
                   "difficulty in classifying the actors in structural equivalence classes."));
    connect(analyzeStrEquivalencePearsonAct, SIGNAL(triggered()),
            this, SLOT(slotAnalyzeStrEquivalencePearsonDialog() )  );



    analyzeStrEquivalenceMatchesAct = new QAction(QIcon(":/images/similarity.png"),
                                            tr("Similarity by measure (Exact, Jaccard, Hamming, Cosine, Euclidean)"),this);
    analyzeStrEquivalenceMatchesAct-> setShortcut(
                QKeySequence(Qt::CTRL + Qt::Key_T, Qt::CTRL + Qt::Key_E)
                );
    analyzeStrEquivalenceMatchesAct->setStatusTip(tr("Compute a pair-wise actor similarity "
                                               "matrix based on a measure of their ties (or distances) \"matches\" ."));
    analyzeStrEquivalenceMatchesAct->setWhatsThis(
                tr("Actor Similarity by measure\n\n"
                   "Computes a pair-wise actor similarity matrix, where each element (i,j) is "
                   "the ratio of tie (or distance) matches of actors i and j to all other actors. \n\n"
                   "SocNetV supports the following matching measures: "
                   "Simple Matching (Exact Matches)"
                   "Jaccard Index (Positive Matches or Co-citation)"
                   "Hamming distance"
                   "Cosine similarity"
                   "Euclidean distance"
                   "For instance, if you select Exact Matches, a matrix element (i,j) = 0.5, "
                   "means that actors i and j have the same ties present or absent "
                   "to other actors 50% of the time. \n\n"
                   "These measures of similarity are particularly useful "
                   "when ties are binary (not valued).\n\n"
                   "Note that in very sparse networks (very low density), measures such as"
                   "\"exact matches\", \"correlation\" and \"distance\" "
                   "will show little variation among the actors, causing "
                   "difficulty in classifying the actors in structural equivalence classes."));
    connect(analyzeStrEquivalenceMatchesAct, SIGNAL(triggered()),
            this, SLOT(slotAnalyzeStrEquivalenceSimilarityMeasureDialog() )  );



    analyzeStrEquivalenceTieProfileDissimilaritiesAct = new QAction(QIcon(":/images/dm.png"),
                                                tr("Tie Profile Dissimilarities/Distances"),this);
    analyzeStrEquivalenceTieProfileDissimilaritiesAct -> setShortcut(
                QKeySequence(Qt::CTRL + Qt::Key_T, Qt::CTRL + Qt::Key_T) );
    analyzeStrEquivalenceTieProfileDissimilaritiesAct->
            setStatusTip(
                tr("Compute tie profile dissimilarities/distances "
                   "(Euclidean, Manhattan, Jaccard, Hamming) between all pair of nodes.")
                );
    analyzeStrEquivalenceTieProfileDissimilaritiesAct->
            setWhatsThis(
                tr("Tie Profile Dissimilarities/Distances\n\n"
                   "Computes a matrix of tie profile distances/dissimilarities "
                   "between all pairs of actors/nodes in the social network "
                   "using an ordinary metric such as Euclidean distance, "
                   "Manhattan distance, Jaccard distance or Hamming distance)."
                   "The resulted distance matrix is a n x n matrix, in which the "
                   "(i,j) element is the distance or dissimilarity between "
                   "the tie profiles of node i and node j."
                   )
                );
    connect(analyzeStrEquivalenceTieProfileDissimilaritiesAct, SIGNAL(triggered()),
            this, SLOT( slotAnalyzeStrEquivalenceDissimilaritiesDialog() ) );


    analyzeStrEquivalenceClusteringHierarchicalAct = new QAction(QIcon(":/images/hierarchical.png"),
                                                                 tr("Hierarchical clustering"),this);
    analyzeStrEquivalenceClusteringHierarchicalAct-> setShortcut(
                QKeySequence(Qt::CTRL + Qt::Key_T, Qt::CTRL + Qt::Key_H));

    analyzeStrEquivalenceClusteringHierarchicalAct->setStatusTip(
                tr("Perform agglomerative cluster analysis of the actors in the social network"));
    analyzeStrEquivalenceClusteringHierarchicalAct->setWhatsThis(
                tr("Hierarchical clustering\n\n"
                   "Hierarchical clustering (or hierarchical cluster analysis, HCA) "
                   "is a method of cluster analysis which builds a hierarchy "
                   "of clusters, based on their elements dissimilarity. "
                   "In SNA context these clusters usually consist of "
                   "network actors. \n"

                   "This method takes the social network distance matrix as input and uses "
                   "the Agglomerative \"bottom up\" approach where each "
                   "actor starts in its own cluster (Level 0). In each subsequent Level, "
                   "as we move up the clustering hierarchy, a pair of clusters "
                   "are merged into a larger cluster, until "
                   "all actors end up in the same cluster. "

                   "To decide which clusters should be combined at each level, a measure of "
                   "dissimilarity between sets of observations is required. "
                   "This measure consists of a metric for the distance between actors "
                   "(i.e. manhattan distance) and a linkage criterion (i.e. single-linkage clustering). "
                   "This linkage criterion (essentially a definition of distance between clusters), "
                   "differentiates between the different HCA methods."

                   "Note that the complexity of agglomerative clustering is O( n^2 log(n) ), "
                   "therefore is too slow for large data sets."
                   ));
    connect(analyzeStrEquivalenceClusteringHierarchicalAct, SIGNAL(triggered()),
            this, SLOT(slotAnalyzeStrEquivalenceClusteringHierarchicalDialog() )  );


    cDegreeAct = new QAction(tr("Degree Centrality (DC)"),this);
    cDegreeAct-> setShortcut(Qt::CTRL + Qt::Key_1);
    cDegreeAct
            ->setStatusTip(tr("Compute Degree Centrality indices for every actor and group Degree Centralization."));
    cDegreeAct
            ->setWhatsThis(
                tr( "Degree Centrality (DC)\n\n"
                    "For each node v, the DC index is the number of edges "
                    "attached to it (in undirected graphs) or the total number "
                    "of arcs (outLinks) starting from it (in digraphs).\n"
                    "This is often considered a measure of actor activity. \n\n"
                    "This index can be calculated in both graphs and digraphs "
                    "but is usually best suited for undirected graphs. "
                    "It can also be calculated in weighted graphs. "
                    "In weighted relations, DC is the sum of weights of all "
                    "edges/outLinks attached to v."));
    connect(cDegreeAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeCentralityDegree()));


    cClosenessAct = new QAction(tr("Closeness Centrality (CC)"), this);
    cClosenessAct-> setShortcut(Qt::CTRL + Qt::Key_2);
    cClosenessAct
            ->setStatusTip(
                tr(
                    "Compute Closeness Centrality indices for every actor and group Closeness Centralization."));
    cClosenessAct
            ->setWhatsThis(
                tr("Closeness Centrality (CC)\n\n"
                   "For each node v, CC the inverse sum of "
                   "the shortest distances between v and every other node. CC is "
                   "interpreted as the ability to access information through the "
                   "\"grapevine\" of network members. Nodes with high closeness "
                   "centrality are those who can reach many other nodes in few steps. "
                   "\n\nThis index can be calculated in both graphs and digraphs. "
                   "It can also be calculated in weighted graphs although the weight of "
                   "each edge (v,u) in E is always considered to be 1. "));
    connect(cClosenessAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeCentralityCloseness()));

    cInfluenceRangeClosenessAct = new QAction(tr("Influence Range Closeness Centrality (IRCC)"), this);
    cInfluenceRangeClosenessAct-> setShortcut(Qt::CTRL + Qt::Key_3);
    cInfluenceRangeClosenessAct
            ->setStatusTip(
                tr("Compute Influence Range Closeness Centrality indices for every actor "
                   "focusing on how proximate each one is"
                   "to others in its influence range"));
    cInfluenceRangeClosenessAct
            ->setWhatsThis(
                tr("Influence Range Closeness Centrality (IRCC)\n\n"
                   "For each node v, IRCC is the standardized inverse average distance "
                   "between v and every reachable node.\n"
                   "This improved CC index is optimized for graphs and directed graphs which "
                   "are not strongly connected. Unlike the ordinary CC, which is the inverted "
                   "sum of distances from node v to all others (thus undefined if a node is isolated "
                   "or the digraph is not strongly connected), IRCC considers only "
                   "distances from node v to nodes in its influence range J (nodes reachable from v). "
                   "The IRCC formula used is the ratio of the fraction of nodes reachable by v "
                   "(|J|/(n-1)) to the average distance of these nodes from v (sum(d(v,j))/|J|"));
    connect(cInfluenceRangeClosenessAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeCentralityClosenessIR()));

    cBetweennessAct = new QAction(tr("Betweenness Centrality (BC)"), this);
    cBetweennessAct-> setShortcut(Qt::CTRL + Qt::Key_4);
    cBetweennessAct->setWhatsThis(tr("Betweenness Centrality (BC)\n\n"
                                     "For each node v, BC is the ratio of all geodesics between pairs of nodes which run through v. "
                                     "It reflects how often an node lies on the geodesics between the other nodes of the network. "
                                     "It can be interpreted as a measure of control. "
                                     "A node which lies between many others is assumed to have a higher likelihood of being able "
                                     "to control information flow in the network. \n\n"
                                     "Note that betweenness centrality assumes that all geodesics "
                                     "have equal weight or are equally likely to be chosen for the flow of information "
                                     "between any two nodes. This is reasonable only on \"regular\" networks where all "
                                     "nodes have similar degrees. On networks with significant degree variance you might want "
                                     "to try informational centrality instead. \n\nThis index can be calculated in both graphs "
                                     "and digraphs but is usually best suited for undirected graphs. It can also be calculated"
                                     " in weighted graphs although the weight of each edge (v,u) in E is always considered to be 1."));
    cBetweennessAct->setStatusTip(tr("Compute Betweenness Centrality indices and group Betweenness Centralization."));
    connect(cBetweennessAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeCentralityBetweenness()));

    cStressAct = new QAction(tr("Stress Centrality (SC)"), this);
    cStressAct-> setShortcut(Qt::CTRL + Qt::Key_5);
    cStressAct->setStatusTip(tr("Compute Stress Centrality indices for every actor and group Stress Centralization."));
    cStressAct->setWhatsThis(tr("Stress Centrality (SC)\n\n"
                                "For each node v, SC is the total number of geodesics between all other nodes which run through v. "
                                "A node with high SC is considered 'stressed', since it is traversed by a high number of geodesics. "
                                "When one node falls on all other geodesics between all the remaining (N-1) nodes, "
                                "then we have a star graph with maximum Stress Centrality. \n\n"
                                "This index can be calculated in both graphs and digraphs but is usually best suited for undirected graphs. "
                                "It can also be calculated in weighted graphs although the weight of each edge (v,u) in E is always considered to be 1."));
    connect(cStressAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeCentralityStress()));


    cEccentAct = new QAction(tr("Eccentricity Centrality (EC)"), this);
    cEccentAct-> setShortcut(Qt::CTRL + Qt::Key_6);
    cEccentAct->setStatusTip(tr("Compute Eccentricity Centrality indices for each node."));
    cEccentAct->setWhatsThis(
                tr("Eccentricity Centrality (EC)\n\n For each node i, "
                   "the EC is the inverse of the maximum geodesic distance "
                   "of that v to all other nodes in the network. \n"
                   "Nodes with high EC have short distances to all other nodes "
                   "This index can be calculated in both graphs and digraphs "
                   "but is usually best suited for undirected graphs. "
                   "It can also be calculated in weighted graphs although the weight of each edge (v,u) in E is always considered to be 1."));
    connect(cEccentAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeCentralityEccentricity()));


    cPowerAct = new QAction(tr("Gil and Schmidt Power Centrality (PC)"), this);
    cPowerAct-> setShortcut(Qt::CTRL + Qt::Key_7);
    cPowerAct->setStatusTip(tr("Compute Power Centrality indices (aka Gil-Schmidt Power Centrality) for every actor and group Power Centralization"));
    cPowerAct->setWhatsThis(tr("Power Centrality (PC)\n\n "
                               "For each node v, this index sums its degree (with weight 1), with the size of the 2nd-order neighbourhood (with weight 2), and in general, with the size of the kth order neighbourhood (with weight k). Thus, for each node in the network the most important other nodes are its immediate neighbours and then in decreasing importance the nodes of the 2nd-order neighbourhood, 3rd-order neighbourhood etc. For each node, the sum obtained is normalised by the total numbers of nodes in the same component minus 1. Power centrality has been devised by Gil-Schmidt. \n\nThis index can be calculated in both graphs and digraphs but is usually best suited for undirected graphs. It can also be calculated in weighted graphs although the weight of each edge (v,u) in E is always considered to be 1 (therefore not considered)."));
    connect(cPowerAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeCentralityPower()));


    cInformationAct = new QAction(tr("Information Centrality (IC)"),	this);
    cInformationAct-> setShortcut(Qt::CTRL + Qt::Key_8);
    cInformationAct->setEnabled(true);
    cInformationAct->setStatusTip(tr("Compute Information Centrality indices and group Information Centralization"));
    cInformationAct->setWhatsThis(
                tr("Information Centrality (IC)\n\n"
                   "Information centrality counts all paths between "
                   "nodes weighted by strength of tie and distance. "
                   "This centrality  measure developed by Stephenson and Zelen (1989) "
                   "focuses on how information might flow through many different paths. \n\n"
                   "This index should be calculated only for  graphs. \n\n"
                   "Note: To compute this index, SocNetV drops all isolated nodes."));
    connect(cInformationAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeCentralityInformation()));


    cEigenvectorAct = new QAction(tr("Eigenvector Centrality (EVC)"),	this);
    cEigenvectorAct-> setShortcut(Qt::CTRL + Qt::Key_9);
    cEigenvectorAct->setEnabled(true);
    cEigenvectorAct->setStatusTip(tr("Compute Eigenvector Centrality indices and group Eigenvector Centralization"));
    cEigenvectorAct->setWhatsThis(
                tr("Eigenvector Centrality (EVC)\n\n"
                   "Computes the Eigenvector centrality of each node in a social network "
                   "which is defined as the ith element of the leading eigenvector "
                   "of the adjacency matrix. The leading eigenvector is the "
                   "eigenvector corresponding to the largest positive eigenvalue."
                   "The Eigenvector Centrality, proposed by Bonacich (1989), is "
                   "an extension of the simpler Degree Centrality because it gives "
                   "each actor a score proportional to the scores of its neighbors. "
                   "Thus, a node may be important, in terms of its EC, because it "
                   "has lots of ties or it has fewer ties to important other nodes."));
    connect(cEigenvectorAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeCentralityEigenvector()));



    cInDegreeAct = new QAction(tr("Degree Prestige (DP)"),	 this);
    cInDegreeAct->setStatusTip(tr("Compute Degree Prestige (InDegree) indices "));
    cInDegreeAct-> setShortcut(Qt::CTRL + Qt::Key_I);
    cInDegreeAct->setWhatsThis(tr("InDegree (Degree Prestige)\n\n"
                                  "For each node k, this the number of arcs ending at k. "
                                  "Nodes with higher in-degree are considered more prominent among others. "
                                  "In directed graphs, this index measures the prestige of each node/actor. "
                                  "Thus it is called Degree Prestige. "
                                  "Nodes who are prestigious tend to receive many nominations or choices (in-links). "
                                  "The largest the index is, the more prestigious is the node. \n\n"
                                  "This index can be calculated only for digraphs. "
                                  "In weighted relations, DP is the sum of weights of all arcs/inLinks ending at node v."));
    connect(cInDegreeAct, SIGNAL(triggered()), this, SLOT(slotAnalyzePrestigeDegree()));

    cPageRankAct = new QAction(tr("PageRank Prestige (PRP)"),	this);
    cPageRankAct-> setShortcut(Qt::CTRL + Qt::Key_K);
    cPageRankAct->setEnabled(true);
    cPageRankAct->setStatusTip(tr("Compute PageRank Prestige indices for every actor"));
    cPageRankAct->setWhatsThis(tr("PageRank Prestige\n\n"
                                  "An importance ranking for each node based on the link structure of the network. "
                                  "PageRank, developed by Page and Brin (1997), focuses on how nodes are "
                                  "connected to each other, treating each edge from a node as a citation/backlink/vote to another. "
                                  "In essence, for each node PageRank counts all backlinks to it, "
                                  "but it does so by not counting all edges equally while it "
                                  "normalizes each edge from a node by the total number of edges from it. "
                                  "PageRank is calculated iteratively and it corresponds to the principal "
                                  "eigenvector of the normalized link matrix. \n\n"
                                  "This index can be calculated in both graphs and digraphs but is "
                                  "usually best suited for directed graphs since it is a prestige measure. "
                                  "It can also be calculated in weighted graphs. "
                                  "In weighted relations, each backlink to a node v from another node u is "
                                  "considered to have weight=1 but it is normalized by the sum of "
                                  "outLinks weights (outDegree) of u. Therefore, nodes with high outLink "
                                  "weights give smaller percentage of their PR to node v."));
    connect(cPageRankAct, SIGNAL(triggered()), this, SLOT(slotAnalyzePrestigePageRank()));

    cProximityPrestigeAct = new QAction(tr("Proximity Prestige (PP)"),	this);
    cProximityPrestigeAct-> setShortcut(Qt::CTRL + Qt::Key_Y);
    cProximityPrestigeAct->setEnabled(true);
    cProximityPrestigeAct->setStatusTip(tr("Calculate and display Proximity Prestige (digraphs only)"));
    cProximityPrestigeAct
            ->setWhatsThis(
                tr("Proximity Prestige (PP) \n\n"
                   "This index measures how proximate a node v is to the nodes "
                   "in its influence domain I (the influence domain I of a node "
                   "is the number of other nodes that can reach it).\n\n"
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
    connect(cProximityPrestigeAct, SIGNAL(triggered()), this, SLOT(slotAnalyzePrestigeProximity()));


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
    connect(optionsNodeNumbersVisibilityAct, SIGNAL(triggered(bool)),
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
    connect(optionsNodeNumbersInsideAct, SIGNAL(triggered(bool)),
            this, SLOT(slotOptionsNodeNumbersInside(bool)));


    optionsNodeLabelsVisibilityAct= new QAction(tr("Display Node Labels"),	this );
    optionsNodeLabelsVisibilityAct->setStatusTip(
                tr("Toggle displaying of node labels (this session only)"));
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


    optionsEdgesVisibilityAct = new QAction(tr("Display Edges"), this);
    optionsEdgesVisibilityAct->setStatusTip(tr("Toggle displaying edges (this session only)"));
    optionsEdgesVisibilityAct->setWhatsThis(
                tr("Display Edges\n\n"
                "Enables or disables displaying of edges"
                "This setting will apply to this session only. \n"
                "To permanently change it, use Settings & Preferences"));
    optionsEdgesVisibilityAct->setCheckable(true);
    optionsEdgesVisibilityAct->setChecked(
                (appSettings["initEdgesVisibility"] == "true") ? true: false
            );
    connect(optionsEdgesVisibilityAct, SIGNAL(triggered(bool)),
            this, SLOT(slotOptionsEdgesVisibility(bool)) );


    optionsEdgeWeightNumbersAct = new QAction(tr("Display Edge Weights"),	this);
    optionsEdgeWeightNumbersAct->setStatusTip(
                tr("Toggle displaying of numbers of edge weights (this session only)"));
    optionsEdgeWeightNumbersAct->setWhatsThis(
                tr("Display Edge Weights\n\n"
                   "Enables or disables displaying edge weight numbers.\n"
                   "This setting will apply to this session only. \n"
                   "To permanently change it, use Settings & Preferences"));
    optionsEdgeWeightNumbersAct->setCheckable(true);
    optionsEdgeWeightNumbersAct->setChecked(
                (appSettings["initEdgeWeightNumbersVisibility"] == "true") ? true: false
                );
    connect(optionsEdgeWeightNumbersAct, SIGNAL(triggered(bool)),
            this, SLOT(slotOptionsEdgeWeightNumbersVisibility(bool)) );

    considerEdgeWeightsAct = new QAction(tr("Consider Edge Weights in Calculations"),	this);
    considerEdgeWeightsAct->
            setStatusTip(
                tr("Toggle considering edge weights during calculations "
                   "(i.e. distances, centrality, etc) (this session only)"));
    considerEdgeWeightsAct->
            setWhatsThis(
                tr("Consider Edge Weights in Calculations\n\n"
                   "Enables or disables considering edge weights during "
                   "calculations (i.e. distances, centrality, etc).\n"
                   "This setting will apply to this session only. \n"
                   "To permanently change it, use Settings & Preferences"));
    considerEdgeWeightsAct->setCheckable(true);
    considerEdgeWeightsAct->setChecked(false);
    connect(considerEdgeWeightsAct, SIGNAL(triggered(bool)),
            this, SLOT(slotOptionsEdgeWeightsDuringComputation(bool)) );


    optionsEdgeLabelsAct = new QAction(tr("Display Edge Labels"),	this);
    optionsEdgeLabelsAct->setStatusTip(
                tr("Toggle displaying of Edge labels, if any (this session only)"));
    optionsEdgeLabelsAct->setWhatsThis(
                tr("Display Edge Labes\n\n"
                   "Enables or disables displaying edge labels.\n"
                   "This setting will apply to this session only. \n"
                   "To permanently change it, use Settings & Preferences"));
    optionsEdgeLabelsAct->setCheckable(true);
    optionsEdgeLabelsAct->setChecked(
                (appSettings["initEdgeLabelsVisibility"] == "true") ? true: false
                );
    connect(optionsEdgeLabelsAct, SIGNAL(triggered(bool)),
            this, SLOT(slotOptionsEdgeLabelsVisibility(bool)) );


    optionsEdgeArrowsAct = new QAction( tr("Display Edge Arrows"),this);
    optionsEdgeArrowsAct->setStatusTip(
                tr("Toggle displaying directional Arrows on edges (this session only)"));
    optionsEdgeArrowsAct->setWhatsThis(
                tr("Display edge Arrows\n\n"
                   "Enables or disables displaying of arrows on edges.\n\n"
                   "Useful if all links are reciprocal (undirected graph).\n"
                   "This setting will apply to this session only. \n"
                   "To permanently change it, use Settings & Preferences"));
    optionsEdgeArrowsAct->setCheckable(true);
    optionsEdgeArrowsAct->setChecked(
                (appSettings["initEdgeArrows"]=="true") ? true: false
                );
    connect(optionsEdgeArrowsAct, SIGNAL(triggered(bool)),
            this, SLOT(slotOptionsEdgeArrowsVisibility(bool)) );

    optionsEdgeThicknessPerWeightAct = new QAction( tr("Edge Thickness reflects Weight"), this);
    optionsEdgeThicknessPerWeightAct->setStatusTip(tr("Draw edges as thick as their weights (if specified)"));
    optionsEdgeThicknessPerWeightAct->setWhatsThis(
                tr("Edge thickness reflects weight\n\n"
                   "Click to toggle having all edges as thick as their weight (if specified)"));
    optionsEdgeThicknessPerWeightAct->setCheckable(true);
    optionsEdgeThicknessPerWeightAct->setChecked(
                (appSettings["initEdgeThicknessPerWeight"]=="true") ? true: false
                                                                      );
    connect(optionsEdgeThicknessPerWeightAct, SIGNAL(triggered(bool)),
            this, SLOT(slotOptionsEdgeThicknessPerWeight(bool)) );
    optionsEdgeThicknessPerWeightAct->setEnabled(false);

    drawEdgesBezier = new QAction( tr("Bezier Curves"),	this);
    drawEdgesBezier->setStatusTip(tr("Draw Edges as Bezier curves"));
    drawEdgesBezier->setWhatsThis(
                tr("Edges Bezier\n\n"
                   "Enable or disables drawing Edges as Bezier curves."
                   "This setting will apply to this session only. \n"
                   "To permanently change it, use Settings & Preferences"));
    drawEdgesBezier->setCheckable(true);
    drawEdgesBezier->setChecked (
                (appSettings["initEdgeShape"]=="bezier") ? true: false
                );
    drawEdgesBezier->setEnabled(false);
    connect(drawEdgesBezier, SIGNAL(triggered(bool)),
            this, SLOT(slotOptionsEdgesBezier(bool)) );


    changeBackColorAct = new QAction(QIcon(":/images/color.png"), tr("Change Background Color"), this);
    changeBackColorAct->setStatusTip(tr("Change the canvasbackground color"));
    changeBackColorAct->setWhatsThis(tr("Background Color\n\n"
                                        "Changes the background color of the canvas"));
    connect(changeBackColorAct, SIGNAL(triggered()),
            this, SLOT(slotOptionsBackgroundColor()));


    backgroundImageAct = new QAction(tr("Background Image (this session)"),	this);
    backgroundImageAct->setStatusTip(
                tr("Select and display a custom image in the background"
                   "(for this session only)"));
    backgroundImageAct->setWhatsThis(
                tr("Background image\n\n"
                   "Enable to select an image file from your computer, "
                   "which will be displayed in the background instead of plain color."
                   "This setting will apply to this session only. \n"
                   "To permanently change it, use Settings & Preferences"));
    backgroundImageAct->setCheckable(true);
    backgroundImageAct->setChecked(false);
    connect(backgroundImageAct, SIGNAL(triggered(bool)),
            this, SLOT(slotOptionsBackgroundImageSelect(bool)));

    openSettingsAct = new QAction(QIcon(":/images/appsettings.png"), tr("Settings"),	this);
    openSettingsAct->setShortcut(Qt::CTRL + Qt::Key_Comma);
    openSettingsAct->setEnabled(true);
    openSettingsAct->setToolTip(
                tr("Open the Settings dialog where you can save your preferences "
                   "for all future sessions"));
    openSettingsAct->setStatusTip(
                tr("Open the Settings dialog to save your preferences "
                   "for all future sessions"));
    openSettingsAct->setWhatsThis(
                tr("Settings\n\n"
                   "Opens the Settings dialog where you can edit and save settings "
                   "permanently for all subsequent sessions."));
    connect(openSettingsAct, SIGNAL(triggered()),
            this, SLOT(slotOpenSettingsDialog()));




    /**
    Help menu actions
    */
    helpApp = new QAction(QIcon(":/images/help.png"), tr("Manual"),	this);
    helpApp -> setShortcut(Qt::Key_F1);
    helpApp->setStatusTip(tr("Read the manual..."));
    helpApp->setWhatsThis(tr("Manual\n\nDisplays the documentation of SocNetV"));
    connect(helpApp, SIGNAL(triggered()), this, SLOT(slotHelp()));

    tipsApp = new QAction(QIcon(":/images/help-hint.png"), tr("Tip of the Day"), this);
    tipsApp->setStatusTip(tr("Read useful tips"));
    tipsApp->setWhatsThis(tr("Quick Tips\n\nDisplays some useful and quick tips"));
    connect(tipsApp, SIGNAL(triggered()), this, SLOT(slotHelpTips()));


    helpCheckUpdatesApp = new QAction(
                QIcon(":/images/download.png"), tr("Check for Updates"),	this);
    helpCheckUpdatesApp->setStatusTip(tr("Open a browser to SocNetV website "
                                         "to check for a new version..."));
    helpCheckUpdatesApp->setWhatsThis(tr("Check Updates\n\n"
                                         "Open a browser to SocNetV website so "
                                         "that you can check yourself for updates"));
    connect(helpCheckUpdatesApp, SIGNAL(triggered()),
            this, SLOT(slotHelpCheckUpdates()));

    helpAboutApp = new QAction(tr("About SocNetV"), this);
    helpAboutApp->setStatusTip(tr("About SocNetV"));
    helpAboutApp->setWhatsThis(tr("About\n\nBasic information about SocNetV"));
    connect(helpAboutApp, SIGNAL(triggered()), this, SLOT(slotHelpAbout()));



    helpAboutQt = new QAction(QIcon(":/images/qt.png"), tr("About Qt"), this);
    helpAboutQt->setStatusTip(tr("About Qt"));
    helpAboutQt->setWhatsThis(tr("About\n\nAbout Qt"));
    connect(helpAboutQt, SIGNAL(triggered()), this, SLOT(slotAboutQt() ) );
}



/**
 * @brief Creates and populates the menu bar
 */
void MainWindow::initMenuBar() {


    /** menuBar entry networkMenu */
    networkMenu = menuBar()->addMenu(tr("&Network"));
    networkMenu -> addAction(networkNew);
    networkMenu -> addAction(networkOpen);
    networkMenu -> addSeparator();
    recentFilesSubMenu = new QMenu(tr("Recent files..."));
    for (int i = 0; i < MaxRecentFiles; ++i)
        recentFilesSubMenu->addAction(recentFileActs[i]);

    slotNetworkFileRecentUpdateActions();

    networkMenu ->addMenu (recentFilesSubMenu );
    networkMenu -> addSeparator();
    importSubMenu = new QMenu(tr("Import ..."));
    importSubMenu -> setIcon(QIcon(":/images/import.png"));
    importSubMenu -> addAction(networkImportGML);
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
    networkMenu -> addAction (networkViewSociomatrixPlotAct);
    networkMenu -> addSeparator();

    networkMenu -> addAction (networkDataSetSelectAct);
    networkMenu -> addSeparator();

    randomNetworkMenu = new QMenu(tr("Create Random Network..."));
    randomNetworkMenu -> setIcon(QIcon(":/images/random.png"));
    networkMenu ->addMenu (randomNetworkMenu);

    randomNetworkMenu -> addAction (createScaleFreeRandomNetworkAct);
    randomNetworkMenu -> addAction (createSmallWorldRandomNetworkAct);
    randomNetworkMenu -> addAction (createErdosRenyiRandomNetworkAct );
    randomNetworkMenu -> addAction (createRegularRandomNetworkAct);
    randomNetworkMenu -> addAction (createLatticeNetworkAct);
    // createGaussianRandomNetworkAct -> addTo(randomNetworkMenu);
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
    editMenu -> addAction (editRelationRenameAct);

    editMenu -> addSeparator();

    editMenu -> addAction ( zoomInAct );
    editMenu -> addAction ( zoomOutAct );

    editMenu -> addSeparator();

    editMenu -> addAction ( editRotateLeftAct );
    editMenu -> addAction ( editRotateRightAct );

    editMenu -> addSeparator();
    editMenu -> addAction (editResetSlidersAct );

    editMenu -> addSeparator();
    editNodeMenu = new QMenu(tr("Nodes..."));
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

    editNodeMenu -> addAction (editNodeSelectedToCliqueAct);
    editNodeMenu -> addAction (editNodeSelectedToStarAct);
    editNodeMenu -> addAction (editNodeSelectedToCycleAct);
    editNodeMenu -> addAction (editNodeSelectedToLineAct);

    editNodeMenu -> addSeparator();

    editNodeMenu -> addAction (editNodeColorAll);
    editNodeMenu -> addAction (editNodeSizeAllAct);
    editNodeMenu -> addAction (editNodeShapeAll);
    editNodeMenu -> addSeparator();
    editNodeMenu -> addAction (editNodeNumbersSizeAct);
    editNodeMenu -> addAction (editNodeNumbersColorAct);
    editNodeMenu -> addSeparator();
    editNodeMenu -> addAction (editNodeLabelsSizeAct);
    editNodeMenu -> addAction (editNodeLabelsColorAct);


    editEdgeMenu = new QMenu(tr("Edges..."));
    editEdgeMenu -> setIcon(QIcon(":/images/line.png"));
    editMenu-> addMenu (editEdgeMenu);
    editEdgeMenu -> addAction(editEdgeAddAct);
    editEdgeMenu -> addAction(editEdgeRemoveAct);
    editEdgeMenu -> addSeparator();
    editEdgeMenu -> addAction (editEdgeSymmetrizeAllAct);
    editEdgeMenu -> addAction (editEdgeSymmetrizeStrongTiesAct);
    editEdgeMenu -> addAction (editEdgesCocitationAct);
    editEdgeMenu -> addAction (editEdgeUndirectedAllAct);
    editEdgeMenu -> addSeparator();
    editEdgeMenu -> addAction(editEdgeLabelAct);
    editEdgeMenu -> addAction(editEdgeColorAct);
    editEdgeMenu -> addAction(editEdgeWeightAct);
    editEdgeMenu -> addSeparator();
    editEdgeMenu -> addAction (editEdgeColorAllAct);

    //   transformNodes2EdgesAct -> addTo (editMenu);

    editMenu ->addSeparator();
    filterMenu = new QMenu ( tr("Filter..."));
    filterMenu -> setIcon(QIcon(":/images/filter.png"));
    editMenu ->addMenu(filterMenu);

    filterMenu -> addAction(filterNodesAct );
    filterMenu -> addAction(editFilterNodesIsolatesAct );
    filterMenu -> addAction(editFilterEdgesByWeightAct );
    filterMenu -> addAction(editFilterEdgesUnilateralAct);


    /** menuBar entry: analyze menu */
    analysisMenu = menuBar()->addMenu(tr("&Analyze"));
    matrixMenu = new QMenu(tr("Adjacency Matrix and Matrices..."));
    matrixMenu -> setIcon(QIcon(":/images/sm.png"));
    analysisMenu -> addMenu (matrixMenu);
    matrixMenu -> addAction (networkViewSociomatrixAct);
    matrixMenu -> addAction (networkViewSociomatrixPlotAct);
    matrixMenu -> addSeparator();
    matrixMenu -> addAction (analyzeMatrixAdjInvertAct);
    matrixMenu -> addSeparator();
    matrixMenu -> addAction(analyzeMatrixAdjTransposeAct);
    matrixMenu -> addSeparator();
    matrixMenu -> addAction(analyzeMatrixAdjCocitationAct);
    matrixMenu -> addSeparator();
    matrixMenu -> addAction (analyzeMatrixDegreeAct);
    matrixMenu -> addAction (analyzeMatrixLaplacianAct);
    //	analysisMenu -> addAction (netDensity);

    analysisMenu -> addSeparator();
    cohesionMenu = new QMenu(tr("Cohesion..."));
    cohesionMenu -> setIcon(QIcon(":/images/distances.png"));
    analysisMenu -> addMenu(cohesionMenu);
    cohesionMenu -> addAction (analyzeGraphReciprocityAct);
    cohesionMenu -> addAction (analyzeGraphSymmetryAct);
    cohesionMenu -> addSection("Graph distances");
    cohesionMenu -> addAction (analyzeGraphDistanceAct);
    cohesionMenu -> addAction (averGraphDistanceAct);
    cohesionMenu -> addSeparator();
    cohesionMenu -> addAction (analyzeMatrixDistancesGeodesicAct);
    cohesionMenu -> addAction (analyzeMatrixGeodesicsAct);
    cohesionMenu -> addSeparator();
    cohesionMenu -> addAction (analyzeGraphEccentricityAct);
    cohesionMenu -> addAction (analyzeGraphDiameterAct);
    cohesionMenu  -> addSeparator();
    cohesionMenu -> addAction(analyzeGraphConnectednessAct);
    cohesionMenu  -> addSeparator();
    cohesionMenu -> addAction (analyzeGraphWalksAct);
    cohesionMenu -> addAction (analyzeGraphWalksTotalAct);
    cohesionMenu  -> addSeparator();
    cohesionMenu -> addAction (analyzeMatrixReachabilityAct);
    cohesionMenu -> addSeparator();
    cohesionMenu -> addAction (clusteringCoefAct);


    analysisMenu->addSeparator();
    // CENTRALITIES
    centrlMenu = new QMenu(tr("Centrality and Prestige indices..."));
    centrlMenu -> setIcon(QIcon(":/images/centrality.png"));
    analysisMenu->addMenu(centrlMenu);
    centrlMenu -> addSection(QIcon(":/images/centrality.png"), tr("Centrality"));

    centrlMenu -> addAction (cDegreeAct);
    centrlMenu -> addAction (cClosenessAct);
    centrlMenu -> addAction (cInfluenceRangeClosenessAct);
    centrlMenu -> addAction (cBetweennessAct);
    centrlMenu -> addAction (cStressAct);
    centrlMenu -> addAction (cEccentAct);
    centrlMenu -> addAction (cPowerAct);
    centrlMenu -> addAction (cInformationAct);
    centrlMenu -> addAction (cEigenvectorAct);
    centrlMenu -> addSection(QIcon(":/images/prestige.png"), tr("Prestige"));
    centrlMenu -> addAction (cInDegreeAct);
    centrlMenu -> addAction (cPageRankAct);
    centrlMenu -> addAction (cProximityPrestigeAct);


    analysisMenu -> addSeparator();
    // COMMUNITIES & SUBGROUPS
    communitiesMenu = new QMenu(tr("Communities and Subgroups..."));
    communitiesMenu -> setIcon(QIcon(":/images/clustering.png"));
    analysisMenu -> addMenu(communitiesMenu);
    communitiesMenu -> addAction (analyzeCommunitiesCliquesAct);
    communitiesMenu -> addSeparator();
    communitiesMenu -> addAction (analyzeCommunitiesTriadCensusAct);


    analysisMenu->addSeparator();
    // STRUCTURAL EQUIVALENCE
    strEquivalenceMenu = new QMenu(tr("Structural Equivalence..."));
    strEquivalenceMenu -> setIcon(QIcon(":/images/similarity.png"));
    analysisMenu -> addMenu (strEquivalenceMenu);
    strEquivalenceMenu -> addAction (analyzeStrEquivalencePearsonAct);
    strEquivalenceMenu -> addAction(analyzeStrEquivalenceMatchesAct);
    strEquivalenceMenu -> addSeparator();
    strEquivalenceMenu -> addAction (analyzeStrEquivalenceTieProfileDissimilaritiesAct);
    strEquivalenceMenu -> addSeparator();
    strEquivalenceMenu -> addAction (analyzeStrEquivalenceClusteringHierarchicalAct);



    /** menuBar entry layoutMenu  */

    layoutMenu = menuBar()->addMenu(tr("&Layout"));
    //   colorationMenu = new QPopupMenu();
    //   layoutMenu -> insertItem (tr("Colorization"), colorationMenu);
    //   strongColorationAct -> addTo(colorationMenu);
    //   regularColorationAct-> addTo(colorationMenu);
    //   layoutMenu->insertSeparator();
    randomLayoutMenu = new QMenu(tr("Random..."));
    layoutMenu -> addMenu (randomLayoutMenu );
    randomLayoutMenu ->  addAction(layoutRandomAct);
    randomLayoutMenu ->  addAction( layoutRandomRadialAct );
    layoutMenu->addSeparator();

    layoutRadialProminenceMenu = new QMenu(tr("Radial by prominence index..."));
    layoutRadialProminenceMenu -> setIcon(QIcon(":/images/circular.png"));
    layoutMenu -> addMenu (layoutRadialProminenceMenu);
    layoutRadialProminenceMenu -> addAction (layoutRadialProminence_DC_Act);
    layoutRadialProminenceMenu -> addAction (layoutRadialProminence_CC_Act);
    layoutRadialProminenceMenu -> addAction (layoutRadialProminence_IRCC_Act);
    layoutRadialProminenceMenu -> addAction (layoutRadialProminence_BC_Act);
    layoutRadialProminenceMenu -> addAction (layoutRadialProminence_SC_Act);
    layoutRadialProminenceMenu -> addAction (layoutRadialProminence_EC_Act);
    layoutRadialProminenceMenu -> addAction (layoutRadialProminence_PC_Act);
    layoutRadialProminenceMenu -> addAction (layoutRadialProminence_IC_Act);
    layoutRadialProminenceMenu -> addAction (layoutRadialProminence_EVC_Act);
    layoutRadialProminenceMenu -> addAction (layoutRadialProminence_DP_Act);
    layoutRadialProminenceMenu -> addAction (layoutRadialProminence_PRP_Act);
    layoutRadialProminenceMenu -> addAction (layoutRadialProminence_PP_Act);

    layoutMenu->addSeparator();

    layoutLevelProminenceMenu = new QMenu (tr("On Levels by prominence index..."));
    layoutLevelProminenceMenu -> setIcon(QIcon(":/images/net3.png"));
    layoutMenu -> addMenu (layoutLevelProminenceMenu);
    layoutLevelProminenceMenu -> addAction (layoutLevelProminence_DC_Act);
    layoutLevelProminenceMenu -> addAction (layoutLevelProminence_CC_Act);
    layoutLevelProminenceMenu -> addAction (layoutLevelProminence_IRCC_Act);
    layoutLevelProminenceMenu -> addAction (layoutLevelProminence_BC_Act);
    layoutLevelProminenceMenu -> addAction (layoutLevelProminence_SC_Act);
    layoutLevelProminenceMenu -> addAction (layoutLevelProminence_EC_Act);
    layoutLevelProminenceMenu -> addAction (layoutLevelProminence_PC_Act);
    layoutLevelProminenceMenu -> addAction (layoutLevelProminence_IC_Act);
    layoutLevelProminenceMenu -> addAction (layoutLevelProminence_EVC_Act);
    layoutLevelProminenceMenu -> addAction (layoutLevelProminence_DP_Act);
    layoutLevelProminenceMenu -> addAction (layoutLevelProminence_PRP_Act);
    layoutLevelProminenceMenu -> addAction (layoutLevelProminence_PP_Act);

    layoutMenu->addSeparator();

    layoutNodeSizeProminenceMenu = new QMenu (tr("Node Size by prominence index..."));
    layoutNodeSizeProminenceMenu -> setIcon(QIcon(":/images/node.png"));
    layoutMenu -> addMenu (layoutNodeSizeProminenceMenu);
    layoutNodeSizeProminenceMenu -> addAction (layoutNodeSizeProminence_DC_Act);
    layoutNodeSizeProminenceMenu -> addAction (layoutNodeSizeProminence_CC_Act);
    layoutNodeSizeProminenceMenu -> addAction (layoutNodeSizeProminence_IRCC_Act);
    layoutNodeSizeProminenceMenu -> addAction (layoutNodeSizeProminence_BC_Act);
    layoutNodeSizeProminenceMenu -> addAction (layoutNodeSizeProminence_SC_Act);
    layoutNodeSizeProminenceMenu -> addAction (layoutNodeSizeProminence_EC_Act);
    layoutNodeSizeProminenceMenu -> addAction (layoutNodeSizeProminence_PC_Act);
    layoutNodeSizeProminenceMenu -> addAction (layoutNodeSizeProminence_IC_Act);
    layoutNodeSizeProminenceMenu -> addAction (layoutNodeSizeProminence_EVC_Act);
    layoutNodeSizeProminenceMenu -> addAction (layoutNodeSizeProminence_DP_Act);
    layoutNodeSizeProminenceMenu -> addAction (layoutNodeSizeProminence_PRP_Act);
    layoutNodeSizeProminenceMenu -> addAction (layoutNodeSizeProminence_PP_Act);

    layoutMenu->addSeparator();

    layoutNodeColorProminenceMenu = new QMenu (tr("Node Color by prominence index..."));
    layoutNodeColorProminenceMenu -> setIcon(QIcon(":/images/nodecolor.png"));
    layoutMenu -> addMenu (layoutNodeColorProminenceMenu);
    layoutNodeColorProminenceMenu -> addAction (layoutNodeColorProminence_DC_Act);
    layoutNodeColorProminenceMenu -> addAction (layoutNodeColorProminence_CC_Act);
    layoutNodeColorProminenceMenu -> addAction (layoutNodeColorProminence_IRCC_Act);
    layoutNodeColorProminenceMenu -> addAction (layoutNodeColorProminence_BC_Act);
    layoutNodeColorProminenceMenu -> addAction (layoutNodeColorProminence_SC_Act);
    layoutNodeColorProminenceMenu -> addAction (layoutNodeColorProminence_EC_Act);
    layoutNodeColorProminenceMenu -> addAction (layoutNodeColorProminence_PC_Act);
    layoutNodeColorProminenceMenu -> addAction (layoutNodeColorProminence_IC_Act);
    layoutNodeColorProminenceMenu -> addAction (layoutNodeColorProminence_EVC_Act);
    layoutNodeColorProminenceMenu -> addAction (layoutNodeColorProminence_DP_Act);
    layoutNodeColorProminenceMenu -> addAction (layoutNodeColorProminence_PRP_Act);
    layoutNodeColorProminenceMenu -> addAction (layoutNodeColorProminence_PP_Act);


    layoutMenu->addSeparator();

    layoutForceDirectedMenu = new QMenu (tr("Force-Directed Placement..."));
    layoutForceDirectedMenu -> setIcon(QIcon(":/images/force.png"));
    layoutMenu -> addMenu (layoutForceDirectedMenu);
    layoutForceDirectedMenu -> addAction (layoutFDP_KamadaKawai_Act);
    layoutForceDirectedMenu -> addAction (layoutFDP_FR_Act);
    layoutForceDirectedMenu -> addAction (layoutFDP_Eades_Act);

    layoutMenu->addSeparator();
    layoutMenu -> addAction (layoutGuidesAct);


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
    edgeOptionsMenu -> addAction (optionsEdgesVisibilityAct);
    edgeOptionsMenu -> addSeparator();
    edgeOptionsMenu -> addAction (optionsEdgeWeightNumbersAct);
    edgeOptionsMenu -> addAction (considerEdgeWeightsAct);
    edgeOptionsMenu -> addAction (optionsEdgeThicknessPerWeightAct);
    edgeOptionsMenu -> addSeparator();
    edgeOptionsMenu -> addAction (optionsEdgeLabelsAct);
    edgeOptionsMenu -> addSeparator();
    edgeOptionsMenu -> addAction (optionsEdgeArrowsAct );
    edgeOptionsMenu -> addSeparator();
    edgeOptionsMenu -> addAction (drawEdgesBezier);

    viewOptionsMenu = new QMenu (tr("&View..."));
    viewOptionsMenu -> setIcon(QIcon(":/images/view.png"));
    optionsMenu -> addMenu (viewOptionsMenu);
    viewOptionsMenu -> addAction (changeBackColorAct);
    viewOptionsMenu -> addAction (backgroundImageAct);

    optionsMenu -> addSeparator();
    optionsMenu -> addAction (openSettingsAct);


    /**  menuBar entry helpMenu */
    helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu -> addAction (helpApp);
    helpMenu -> addAction (tipsApp);
    helpMenu -> addSeparator();
    helpMenu -> addAction (helpCheckUpdatesApp);
    helpMenu -> addSeparator();
    helpMenu-> addAction (helpAboutApp);
    helpMenu-> addAction (helpAboutQt);

}




/**
 * @brief Initializes the toolbar
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
    labelRelationSelect ->setText(tr("Relations:"));
    toolBar -> addWidget (labelRelationSelect);
    toolBar -> addAction (editRelationPreviousAct);
    editRelationChangeCombo = new QComboBox;
    editRelationChangeCombo ->setEditable(true);
    editRelationChangeCombo ->setInsertPolicy(QComboBox::InsertAtCurrent);
    editRelationChangeCombo->setMinimumWidth(180);
    editRelationChangeCombo->setCurrentIndex(0);
    editRelationChangeCombo->setToolTip(
                tr("Current relation. To rename it, write new name and press Enter."));
    editRelationChangeCombo->setStatusTip(
                tr("Name of the current relation. "
                   "To rename it, write a new name and press Enter. To select another relation use Down arrow"));
    editRelationChangeCombo->setWhatsThis(
                tr("Relations combo\n\n"
                   "This combo box displays the current relation. \n"
                   "To rename the current relation, write a new name and press Enter. "
                   "To select another relation (if any), click the Down arrow."));

    toolBar -> addWidget(editRelationChangeCombo);
    toolBar -> addAction (editRelationNextAct);
    toolBar -> addAction (editRelationAddAct);

    toolBar -> addSeparator();
    QLabel *labelEditNodes= new QLabel;
    labelEditNodes ->setText(tr("Nodes:"));
    toolBar -> addWidget (labelEditNodes);
    toolBar -> addAction (editNodeAddAct);
    toolBar -> addAction (editNodeRemoveAct);
    toolBar -> addAction (editNodeFindAct);
    toolBar -> addAction(editNodePropertiesAct );
    toolBar -> addSeparator();
    QLabel *labelEditEdges= new QLabel;
    labelEditEdges ->setText(tr("Edges:"));
    toolBar -> addWidget (labelEditEdges);

    toolBar -> addAction (editEdgeAddAct);
    toolBar -> addAction (editEdgeRemoveAct);
    toolBar -> addAction (editFilterEdgesByWeightAct);
    toolBar -> addSeparator();
    QLabel *labelApplicationIcons = new QLabel;
    labelApplicationIcons ->setText(tr("Settings:"));
    toolBar -> addWidget(labelApplicationIcons);
    toolBar -> addAction(openSettingsAct);
    toolBar -> addSeparator();
    toolBar -> addAction ( QWhatsThis::createAction (this));
    toolBar -> setIconSize(QSize(16,16));
}








/**
 * @brief Creates docked panels for instant access to main app functionalities
 * and displaying statistics
 */
void MainWindow::initPanels(){

    /*
     *  create widgets for the Control Panel
     */

    QLabel *toolBoxEditNodeSubgraphSelectLabel  = new QLabel;
    toolBoxEditNodeSubgraphSelectLabel->setText(tr("Selection Subgraph:"));
    toolBoxEditNodeSubgraphSelectLabel->setMinimumWidth(115);
    toolBoxEditNodeSubgraphSelect = new QComboBox;
    toolBoxEditNodeSubgraphSelect->setStatusTip(
                tr("Create a basic subgraph with selected nodes."));
    toolBoxEditNodeSubgraphSelect->setToolTip(
                tr("Create a basic subgraph (star, clique, line, etc) "
                   "with selected nodes. \n"
                   "There must be some nodes selected!"));
    toolBoxEditNodeSubgraphSelect->setWhatsThis(
                        tr("Selection Subgraph\n\n"
                           "Creates basic subgraphs with all selected nodes: star, clique, line, etc."));
    QStringList editNodeSubgraphCommands;
    editNodeSubgraphCommands << "Select"
                       << "Clique"
                       << "Star"
                       << "Cycle"
                       << "Line";
    toolBoxEditNodeSubgraphSelect->addItems(editNodeSubgraphCommands);
    toolBoxEditNodeSubgraphSelect->setMinimumWidth(115);

    QLabel *toolBoxEdgeModeSelectLabel  = new QLabel;
    toolBoxEdgeModeSelectLabel->setText(tr("Edge Mode:"));
    toolBoxEdgeModeSelectLabel->setMinimumWidth(115);
    toolBoxEditEdgeModeSelect = new QComboBox;
    toolBoxEditEdgeModeSelect->setStatusTip(
                tr("Select an edge creation mode: directed or undirected."));
    toolBoxEditEdgeModeSelect->setToolTip(
                tr("Select an edge creation mode: directed or undirected."));
            toolBoxEditEdgeModeSelect->setWhatsThis(
                        tr("Edge mode\n\n"
                           "Select what mode to use when creating new edges."));
    QStringList edgeModeCommands;
    edgeModeCommands << "Directed"
                     << "Undirected";
    toolBoxEditEdgeModeSelect->addItems(edgeModeCommands);
    toolBoxEditEdgeModeSelect->setMinimumWidth(115);


    QLabel *toolBoxSymmetrizeSelectLabel  = new QLabel;
    toolBoxSymmetrizeSelectLabel->setText(tr("Symmetrize:"));
    toolBoxSymmetrizeSelectLabel->setMinimumWidth(115);
    toolBoxEditEdgeSymmetrizeSelect = new QComboBox;
    toolBoxEditEdgeSymmetrizeSelect->setStatusTip(
                tr("Select a method to symmetrize the network, i.e. tranform all directed edges to undirected."));
    toolBoxEditEdgeSymmetrizeSelect->setToolTip(
                        tr("Select a method to symmetrize the network: \n\n"
                           "Symmetrize Directed Edges:\n"
                           "Makes all directed arcs in this relation reciprocal. \n"
                           "That is, if there is an arc from node A to node B \n"
                           "then a new arc from node B to node A is created \n"
                           "with the same weight.\n\n"

                           "Symmetrize Edges by examining Strong Ties:\n"
                           "Creates a new symmetric relation by keeping strong ties only. \n"
                           "In the new relation, a tie will exist between actor A and \n"
                           "actor B only when both arcs A -> B and B -> A are present \n"
                           "in the current or all relations. \n\n"

                           "Symmetrize Edges by examining Cocitation:\n"
                           "Creates a new symmetric relation by connecting actors \n"
                           "that are cocitated by others. \n"
                           "In the new relation, an edge will exist between actor i and \n"
                           "actor j only if C(i,j) > 0, where C the Cocitation Matrix. "
                           ));

    toolBoxEditEdgeSymmetrizeSelect->setWhatsThis(
                        tr("Select a method to symmetrize the network: \n\n"
                           "Symmetrize Directed Edges\n"
                           "Makes all directed arcs in this relation reciprocal. "
                           "That is, if there is an arc from node A to node B \n"
                           "then a new arc from node B to node A is created \n"
                           "with the same weight.\n\n"

                           "Symmetrize Edges by examining Strong Ties:\n"
                           "Creates a new symmetric relation by keeping strong ties only. "
                           "That is, a strong tie exists between actor A and actor B "
                           "only when both arcs A -> B and B -> A are present. "
                           "If the network is multi-relational, it asks you whether "
                           "ties in the current relation or all relations are to be considered. \n\n"

                           "Symmetrize Edges by examining Cocitation:\n"
                           "Creates a new symmetric relation by connecting actors "
                           "that are cocitated by others. "
                           "In the new relation, an edge will exist between actor i and "
                           "actor j only if C(i,j) > 0, where C the Cocitation Matrix. "
                           "Thus the actor pairs cited by more common neighbors will appear "
                           "with a stronger tie between them than pairs those cited by fewer "
                           "common neighbors. "

                           ));
    QStringList symmetrizeCommands;
    symmetrizeCommands << "Select"
                       << "Directed ties"
                       << "Strong ties"
                       << "Cocitation";
    toolBoxEditEdgeSymmetrizeSelect->addItems(symmetrizeCommands);
    toolBoxEditEdgeSymmetrizeSelect->setMinimumWidth(115);


    //create a grid layout for Edit buttons
    QGridLayout *editNodesGrid = new QGridLayout;
    editNodesGrid -> addWidget(toolBoxEditNodeSubgraphSelectLabel, 0,0);
    editNodesGrid -> addWidget(toolBoxEditNodeSubgraphSelect, 0,1);

    QGroupBox *editNodesGroupBox= new QGroupBox(tr("Nodes"));
    editNodesGroupBox->setLayout(editNodesGrid);

    QGridLayout *editEdgeGrid = new QGridLayout;
    editEdgeGrid -> addWidget(toolBoxEdgeModeSelectLabel,0,0);
    editEdgeGrid -> addWidget(toolBoxEditEdgeModeSelect,0,1);
    editEdgeGrid -> addWidget(toolBoxSymmetrizeSelectLabel,1,0);
    editEdgeGrid -> addWidget(toolBoxEditEdgeSymmetrizeSelect,1,1);

    QGroupBox *editEdgeGroupBox= new QGroupBox(tr("Edges"));
    editEdgeGroupBox->setLayout(editEdgeGrid);

    QGridLayout *EditGrid = new QGridLayout;
    EditGrid -> addWidget(editNodesGroupBox, 0,0,1,2);
    EditGrid -> addWidget(editEdgeGroupBox,1,0,1,2);
    EditGrid -> setSpacing(5);
    EditGrid -> setContentsMargins(5, 5, 5, 5);

    //create a groupbox "Edit" - Inside, display the grid layout of widgets
    QGroupBox *editGroupBox= new QGroupBox(tr("Edit"));
    editGroupBox->setLayout(EditGrid);
    editGroupBox->setMaximumWidth(280);
    editGroupBox->setMinimumHeight(100);


    //create widgets for the "Analysis" box
    QLabel *toolBoxAnalysisMatricesSelectLabel = new QLabel;
    toolBoxAnalysisMatricesSelectLabel->setText(tr("Matrix:"));
    toolBoxAnalysisMatricesSelectLabel->setMinimumWidth(115);
    toolBoxAnalysisMatricesSelect = new QComboBox;
    toolBoxAnalysisMatricesSelect -> setStatusTip(
                tr("Select which matrix to compute and display, based on the adjacency matrix of the current network."));
    toolBoxAnalysisMatricesSelect -> setToolTip(
                tr("The adjacency matrix and other matrices based on the adjacency \n"
                   "matrix of the current network, i.e. Cocitation, Degree Matrix etc."));
    toolBoxAnalysisMatricesSelect -> setWhatsThis(
                tr("Analyze Matrices\n\n"
                   "Compute and display matrices based on the adjacency matrix of the current network."));
    QStringList graphMatricesList;
    graphMatricesList << "Select"
                          << "Adjacency"
                          << "Adjacency Plot"
                          << "Adjacency Inverse"
                          << "Adjacency Transpose"
                          << "Cocitation Matrix"
                          << "Degree Matrix"
                          << "Laplacian Matrix";
    toolBoxAnalysisMatricesSelect->addItems(graphMatricesList);
    toolBoxAnalysisMatricesSelect->setMinimumWidth(115);



    QLabel *toolBoxAnalysisCohesionSelectLabel = new QLabel;
    toolBoxAnalysisCohesionSelectLabel->setText(tr("Cohesion:"));
    toolBoxAnalysisCohesionSelectLabel->setMinimumWidth(115);
    toolBoxAnalysisCohesionSelect = new QComboBox;
    toolBoxAnalysisCohesionSelect -> setStatusTip(
                tr("Select a graph-theoretic metric to compute, i.e. distances, walks, graph diameter, eccentricity."));
    toolBoxAnalysisCohesionSelect -> setToolTip(
                tr("Basic graph-theoretic metrics, such as distances, walks, \n"
                   "graph diameter, eccentricity, clustering coefficient, etc."));
    toolBoxAnalysisCohesionSelect -> setWhatsThis(
                tr("Analyze Cohesion\n\n"
                   "Compute basic graph-theoretic metrics, i.e. distances, walks, graph diameter, eccentricity."));
    QStringList graphPropertiesList;
    graphPropertiesList << "Select"
                          << "Reciprocity"
                          << "Symmetry"
                          << "Distance"
                          << "Average Distance"
                          << "Distances Matrix"
                          << "Geodesics Matrix"
                          << "Eccentricity"
                          << "Diameter"
                          << "Connectedness"
                          << "Walks of given length"
                          << "Total Walks"
                          << "Reachability Matrix"
                          << "Clustering Coefficient";
    toolBoxAnalysisCohesionSelect->addItems(graphPropertiesList);
    toolBoxAnalysisCohesionSelect->setMinimumWidth(115);



    QLabel *toolBoxAnalysisProminenceSelectLabel  = new QLabel;
    toolBoxAnalysisProminenceSelectLabel->setText(tr("Prominence:"));
    toolBoxAnalysisProminenceSelectLabel->setMinimumWidth(115);
    toolBoxAnalysisProminenceSelect = new QComboBox;
    toolBoxAnalysisProminenceSelect -> setStatusTip(
                tr("Select a prominence metric to compute for each actor and the whole network. ")
                );
    toolBoxAnalysisProminenceSelect -> setToolTip(
                tr("Metrics to understand how 'prominent' or important each \n"
                   "actor (node) is inside the network, i.e.\n Betweeness Centrality, \n"
                   "Eigenvector Centrality, PageRank etc.")
                );
    toolBoxAnalysisProminenceSelect -> setWhatsThis(
                tr("Analyze Prominence\n\n"
                   "Computes various metrics to see how 'prominent' or "
                   "important each actor (node) is inside the network.\n\n"
                "Centrality metrics quantify how central is each node by examining "
                   "its ties and its geodesic distances (shortest path lengths) to other nodes. "
                "Most Centrality indices were designed for undirected graphs.\n\n"
                "Prestige indices focus on \"choices received\" to a node. "
                "These indices measure the nominations or ties to each node from all others (or inLinks). "
                "Prestige indices are suitable (and can be calculated only) on directed graphs.")
                );
    QStringList prominenceCommands;
    prominenceCommands << "Select"
                       << "Degree Centr."
                       << "Closeness Centr."
                       << "IR Closeness Centr."
                       << "Betweenness Centr."
                       << "Stress Centr."
                       << "Eccentricity Centr."
                       << "Power Centr."
                       << "Information Centr."
                       << "Eigenvector Centr"
                       << "Degree Prestige"
                       << "PageRank Prestige"
                       << "Proximity Prestige";
    toolBoxAnalysisProminenceSelect->addItems(prominenceCommands);
    toolBoxAnalysisProminenceSelect->setMinimumWidth(115);


    QLabel *toolBoxAnalysisCommunitiesSelectLabel  = new QLabel;
    toolBoxAnalysisCommunitiesSelectLabel->setText(tr("Communities:"));
    toolBoxAnalysisCommunitiesSelectLabel->setMinimumWidth(115);
    toolBoxAnalysisCommunitiesSelect = new QComboBox;
    toolBoxAnalysisCommunitiesSelect->setStatusTip(
                tr("Select a community detection metric / cohesive subgroup algorithm, i.e. cliques, triad census etc."));
    toolBoxAnalysisCommunitiesSelect->setToolTip(
                tr("Community detection metrics and cohesive subgroup algorithms, \n"
                   "i.e. cliques, triad census etc."));
            toolBoxAnalysisCommunitiesSelect->setWhatsThis(
                        tr("Analyze Communities\n\n"
                           "Community detection metrics and cohesive subgroup algorithms, "
                           "(i.e. cliques, triad census etc), to identify "
                           "meaningful subgraphs in the graph."
                           "For instance, select cliques to count and identify "
                           "all maximal cliques of actors in the network. "));
    QStringList communitiesCommands;
    communitiesCommands << "Select"
                           << "Cliques"
                           << "Triad Census";
    toolBoxAnalysisCommunitiesSelect->addItems(communitiesCommands);
    toolBoxAnalysisCommunitiesSelect->setMinimumWidth(115);




    QLabel *toolBoxAnalysisStrEquivalenceSelectLabel  = new QLabel;
    toolBoxAnalysisStrEquivalenceSelectLabel->setText(tr("Equivalence:"));
    toolBoxAnalysisStrEquivalenceSelectLabel->setMinimumWidth(115);
    toolBoxAnalysisStrEquivalenceSelect = new QComboBox;
    toolBoxAnalysisStrEquivalenceSelect->setStatusTip(
                tr("Select a metric to measure structural equivalence, "
                   "i.e. Pearson Coefficients, tie profile similarities, "
                   "hierarchical clustering, etc."));
    toolBoxAnalysisStrEquivalenceSelect->setToolTip(
                tr("Structural equivalence measures and visualization algorithms, \n"
                   "i.e. Pearson Coefficients, tie profile similarities, \n"
                   "hierarchical clustering"));
    toolBoxAnalysisStrEquivalenceSelect->setWhatsThis(
                tr("Analyze Structural Equivalence\\n\n"
                   "Structural equivalence measures and visualization algorithms, "
                   "i.e. Pearson Coefficients, tie profile similarities, "
                   "hierarchical clustering "));
    QStringList connectivityCommands;
    connectivityCommands << "Select"
                         << "Pearson Coefficients"
                         << "Similarities"
                         << "Dissimilarities"
                         << "Hierarchical Clustering";
    toolBoxAnalysisStrEquivalenceSelect->addItems(connectivityCommands);
    toolBoxAnalysisStrEquivalenceSelect->setMinimumWidth(115);


    //create layout for analysis options
    QGridLayout *analysisGrid = new QGridLayout();
    analysisGrid -> addWidget(toolBoxAnalysisMatricesSelectLabel, 0,0);
    analysisGrid -> addWidget(toolBoxAnalysisMatricesSelect, 0,1);
    analysisGrid -> addWidget(toolBoxAnalysisCohesionSelectLabel, 1,0);
    analysisGrid -> addWidget(toolBoxAnalysisCohesionSelect, 1,1);
    analysisGrid -> addWidget(toolBoxAnalysisProminenceSelectLabel, 2,0);
    analysisGrid -> addWidget(toolBoxAnalysisProminenceSelect, 2,1);
    analysisGrid -> addWidget(toolBoxAnalysisCommunitiesSelectLabel, 3,0);
    analysisGrid -> addWidget(toolBoxAnalysisCommunitiesSelect, 3,1);
    analysisGrid -> addWidget(toolBoxAnalysisStrEquivalenceSelectLabel, 4,0);
    analysisGrid -> addWidget(toolBoxAnalysisStrEquivalenceSelect, 4,1);

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
                   "Apply a prominence-based layout model to the network. \n"
                   "For instance, you can apply a degree centrality layout. "
                   "For each prominence index, you can select a radial or level layout type."));
    QStringList indicesList;
    indicesList << "None"
                << "Random"
                << "Degree Centr."
                << "Closeness Centr."
                << "IR Closeness Centr."
                << "Betweenness Centr."
                << "Stress Centr."
                << "Eccentricity Centr."
                << "Power Centr."
                << "Information Centr."
                << "Eigenvector Centr."
                << "Degree Prestige"
                << "PageRank Prestige"
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
                tr("Select radial or level layout type (you must select an index above)"));
    toolBoxLayoutByIndexTypeSelect->setWhatsThis(
                tr("Layout Type\n\n"
                   "Select a layout type (radial or level) for the selected prominence-based model "
                   "you want to apply to the network."));
    QStringList layoutTypes;
    layoutTypes << "Radial" << "On Levels" << "Node Size"<< "Node Color";
    toolBoxLayoutByIndexTypeSelect->addItems(layoutTypes);
    toolBoxLayoutByIndexTypeSelect->setMinimumHeight(20);
    toolBoxLayoutByIndexTypeSelect->setMinimumWidth(120);

    toolBoxLayoutByIndexApplyButton = new QPushButton(tr("Apply"));
    toolBoxLayoutByIndexApplyButton->setFocusPolicy(Qt::NoFocus);
    toolBoxLayoutByIndexApplyButton->setMinimumHeight(20);
    toolBoxLayoutByIndexApplyButton->setMaximumWidth(60);


    //create layout for visualisation by index options
    QGridLayout *layoutByIndexGrid = new QGridLayout();
    layoutByIndexGrid -> addWidget(toolBoxLayoutByIndexSelectLabel, 0,0);
    layoutByIndexGrid -> addWidget(toolBoxLayoutByIndexSelect, 0,1);
    layoutByIndexGrid -> addWidget(toolBoxLayoutByIndexTypeLabel, 1,0);
    layoutByIndexGrid -> addWidget(toolBoxLayoutByIndexTypeSelect, 1,1);
    layoutByIndexGrid -> addWidget(toolBoxLayoutByIndexApplyButton, 2,1);
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
                << tr("Kamada-Kawai")
                << tr("Fruchterman-Reingold")
                << tr("Eades Spring Embedder")
                ;

    toolBoxLayoutForceDirectedSelect->addItems(modelsList);
    toolBoxLayoutForceDirectedSelect->setMinimumHeight(20);
    toolBoxLayoutForceDirectedSelect->setMinimumWidth(120);
    toolBoxLayoutForceDirectedSelect->setStatusTip (
                            tr("Select a Force-Directed layout model. "));
    toolBoxLayoutForceDirectedSelect->setToolTip (
                tr("Select a Force-Directed Placement layout model to embed to the network.\n"
                   "Available models: \n\n"
                   "Eades: A spring-gravitational model, the oldest one, where \n"
                   "connected nodes attract each other and all nodes repel all \n"
                   "other non-adjacent nodes. \n\n"

                   "Fruchterman-Reingold: Similar to Eades Spring Embedder but more efficient. \n"
                   "Again adjacent vertices attract each each other but, unlike "
                   "Eades, all vertices repel each other.\n\n"

                   "Kamada-Kawai\n"
                   "The most efficient model of the Spring Embedder family, where \n"
                   "the optimal layout is that of minimum total spring energy, \n"
                   "which is computed as the square summation of the differences \n"
                   "between desirable distances and real ones for all pairs of vertices."
                   )
                );
    toolBoxLayoutForceDirectedSelect->setWhatsThis(
                tr("Visualize by a Force-Directed Placement layout model.\n\n"
                   "Available models: \n\n"
                   "Eades Spring Embedder\n"
                   "A spring-gravitational model, where each node is "
                   "regarded as physical object (ring) repelling all other non-adjacent"
                   "nodes, while springs between connected nodes attract them. \n\n"

                   "Fruchterman-Reingold\n"
                   "In this model, the vertices behave as atomic particles "
                   "or celestial bodies, exerting attractive and repulsive "
                   "forces to each other. Again, only vertices that are "
                   "neighbours  attract each other but, unlike Eades Spring "
                   "Embedder, all vertices repel each other.\n\n"

                   "Kamada-Kawai\n"
                   "The best variant of the Spring Embedder family of models. "
                   "In this the graph is considered to be a dynamic system where "
                   "every edge is between two actors is a 'spring' of a desirable "
                   "length, which corresponds to their graph theoretic distance. \n"
                   "In this way, the optimal layout of the graph "
                   "is the state with the minimum imbalance. The degree of "
                   "imbalance is formulated as the total spring energy: "
                   "the square summation of the differences between desirable "
                   "distances and real ones for all pairs of vertices."
                   )
                );

    toolBoxLayoutForceDirectedApplyButton = new QPushButton(tr("Apply"));
    toolBoxLayoutForceDirectedApplyButton->setFocusPolicy(Qt::NoFocus);
    toolBoxLayoutForceDirectedApplyButton->setMinimumHeight(20);
    toolBoxLayoutForceDirectedApplyButton->setMaximumWidth(60);

    //create layout for dynamic visualisation
    QGridLayout *layoutForceDirectedGrid = new QGridLayout();
    layoutForceDirectedGrid -> addWidget(toolBoxLayoutForceDirectedSelectLabel, 0,0);
    layoutForceDirectedGrid -> addWidget(toolBoxLayoutForceDirectedSelect, 0,1);
    layoutForceDirectedGrid -> addWidget(toolBoxLayoutForceDirectedApplyButton, 1,1);
    layoutForceDirectedGrid -> setSpacing(5);
    layoutForceDirectedGrid -> setContentsMargins(5,5, 5, 5);

    //create a box for dynamic layout options
    QGroupBox *layoutDynamicBox= new QGroupBox(tr("By Force-Directed Model"));
    layoutDynamicBox->setMinimumHeight(90);
    layoutDynamicBox->setLayout (layoutForceDirectedGrid );



    //Parent box with vertical layout for all layout/visualization boxes
    QVBoxLayout *visualizationBoxLayout = new QVBoxLayout;
    visualizationBoxLayout -> addWidget(layoutByIndexBox);
    visualizationBoxLayout -> addWidget(layoutDynamicBox);


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

    //create widgets for Properties/Statistics group/tab

    rightPanelNetworkTypeLabel = new QLabel;
    rightPanelNetworkTypeLabel-> setText ("Network Type: Undirected");
    rightPanelNetworkTypeLabel->setStatusTip(tr("Directed data mode. Toggle the menu option Edit -> Edges -> Undirected Edges to change it"));

    rightPanelNetworkTypeLabel->setToolTip(tr("The loaded network, if any, is directed and \n"
                                "any link you add between nodes will be a directed arc.\n"
                                "If you want to work with undirected edges and/or \n"
                                "transform the loaded network (if any) to undirected \n"
                                "toggle the option Edit -> Edges -> Undirected \n"
                                "or press CTRL+E+U"));
    rightPanelNetworkTypeLabel->setWhatsThis(tr("The loaded network, if any, is directed and \n"
                                "any link you add between nodes will be a directed arc.\n"
                                "If you want to work with undirected edges and/or \n"
                                "transform the loaded network (if any) to undirected \n"
                                "toggle the option Edit -> Edges -> Undirected \n"
                                "or press CTRL+E+U"));

    QFont labelFont = rightPanelNetworkTypeLabel ->font();
    labelFont.setWeight(QFont::Bold);
    rightPanelNetworkTypeLabel ->setFont(labelFont);
    rightPanelNetworkTypeLabel ->setFixedWidth(195);


    QLabel *rightPanelNodesLabel = new QLabel;
    rightPanelNodesLabel->setText(tr("Total Nodes"));
    rightPanelNodesLabel->setStatusTip(tr("The total number of actors (nodes or vertices) in the social network."));
    rightPanelNodesLabel->setToolTip(tr("The total number of actors \n"
                                        "(nodes or vertices) in the social network."));

    rightPanelNodesLCD=new QLCDNumber(7);
    rightPanelNodesLCD->setSegmentStyle(QLCDNumber::Flat);
    rightPanelNodesLCD->setStatusTip(tr("The total number of actors (nodes or vertices) in the social network."));
    rightPanelNodesLCD->setToolTip(tr("This is the total number of actors \n"
                                      "(nodes or vertices) in the social network."));

    rightPanelEdgesLabel = new QLabel;
    rightPanelEdgesLabel->setText(tr("Total Arcs"));
    rightPanelEdgesLabel->setStatusTip(tr("The total number of edges (links between actors) in the social network."));
    rightPanelEdgesLabel->setToolTip(tr("This is the total number of edges \n"
                                        "(links between actors) in the social network."));

    rightPanelEdgesLCD=new QLCDNumber(7);
    rightPanelEdgesLCD->setSegmentStyle(QLCDNumber::Flat);
    rightPanelEdgesLCD->setStatusTip(tr("The total number of directed edges in the social network."));
    rightPanelEdgesLCD->setToolTip(tr("This is the total number of directed edges \n"
                                        "(links between actors) in the social network."));


    QLabel *rightPanelDensityLabel = new QLabel;
    rightPanelDensityLabel->setText(tr("Density"));
    rightPanelDensityLabel->setToolTip(tr("The density of a social network is the ratio of existing \n"
                                  "edges to all possible edges ( n*(n-1) ) between nodes."));
    rightPanelDensityLCD=new QLCDNumber(7);
    rightPanelDensityLCD->setSegmentStyle(QLCDNumber::Flat);
    rightPanelDensityLCD->setStatusTip(tr("The network density, the ratio of existing "
                                "edges to all possible edges ( n*(n-1) ) between nodes."));
    rightPanelDensityLCD->setToolTip(tr("This is the density of the network. \n"
                              "The density of a network is the ratio of existing \n"
                              "edges to all possible edges ( n*(n-1) ) between nodes."));



    QLabel *verticalSpaceLabel1 = new QLabel;
    verticalSpaceLabel1 -> setText ("");
    QLabel *rightPanelSelectedHeaderLabel = new QLabel;
    rightPanelSelectedHeaderLabel-> setText (tr("Selection"));
    rightPanelSelectedHeaderLabel->setFont(labelFont);

    QLabel *rightPanelSelectedNodesLabel = new QLabel;
    rightPanelSelectedNodesLabel->setText(tr("Selected Nodes"));
    rightPanelSelectedNodesLabel->setStatusTip(tr("The number of selected nodes (vertices)."));
    rightPanelSelectedNodesLabel->setToolTip(tr("The number of selected nodes (vertices)."));
    rightPanelSelectedNodesLCD=new QLCDNumber(7);
    rightPanelSelectedNodesLCD->setSegmentStyle(QLCDNumber::Flat);
    rightPanelSelectedNodesLCD->setStatusTip(tr("The number of selected nodes (vertices)."));
    rightPanelSelectedNodesLCD->setToolTip(tr("The number of selected nodes (vertices)."));

    rightPanelSelectedEdgesLabel = new QLabel;
    rightPanelSelectedEdgesLabel->setText(tr("Selected Arcs"));
    rightPanelSelectedEdgesLabel->setStatusTip(tr("The number of selected edges."));
    rightPanelSelectedEdgesLabel->setToolTip(tr("The number of selected edges."));
    rightPanelSelectedEdgesLCD=new QLCDNumber(7);
    rightPanelSelectedEdgesLCD->setSegmentStyle(QLCDNumber::Flat);
    rightPanelSelectedEdgesLCD->setStatusTip(tr("The number of selected edges."));
    rightPanelSelectedEdgesLCD->setToolTip(tr("The number of selected edges."));


    QLabel *verticalSpaceLabel2 = new QLabel;
    verticalSpaceLabel2-> setText ("");

    QLabel *rightPanelClickedNodeHeaderLabel = new QLabel;
    rightPanelClickedNodeHeaderLabel-> setText (tr("Clicked Node"));
    rightPanelClickedNodeHeaderLabel->setFont(labelFont);

    QLabel *rightPanelClickedNodeLabel = new QLabel;
    rightPanelClickedNodeLabel -> setText (tr("Number:"));
    rightPanelClickedNodeLabel -> setToolTip (tr("The node number of the last clicked node."));
    rightPanelClickedNodeLabel -> setStatusTip( tr("The node number of the last clicked node. Zero means no node clicked."));
    rightPanelClickedNodeLCD =new QLCDNumber(5);
    rightPanelClickedNodeLCD ->setSegmentStyle(QLCDNumber::Flat);
    rightPanelClickedNodeLCD -> setToolTip (tr("This is the node number of the last clicked node. \n"
                                               "Becomes zero when you click on something other than a node."));
    rightPanelClickedNodeLCD -> setStatusTip( tr("The node number of the last clicked node. Zero if you clicked something else."));

    QLabel *rightPanelClickedNodeInDegreeLabel = new QLabel;
    rightPanelClickedNodeInDegreeLabel -> setText (tr("In-Degree:"));
    rightPanelClickedNodeInDegreeLabel -> setToolTip (tr("The inDegree of a node is the sum of all inbound edge weights."));
    rightPanelClickedNodeInDegreeLabel -> setStatusTip (tr("The inDegree of a node is the sum of all inbound edge weights."));
    rightPanelClickedNodeInDegreeLCD=new QLCDNumber(5);
    rightPanelClickedNodeInDegreeLCD -> setSegmentStyle(QLCDNumber::Flat);
    rightPanelClickedNodeInDegreeLCD -> setStatusTip (tr("The sum of all inbound edge weights of the last clicked node. "
                                                       "Zero if you clicked something else."));
    rightPanelClickedNodeInDegreeLCD -> setToolTip (tr("This is the sum of all inbound edge weights of last clicked node. \n"
                                                         "Becomes zero when you click on something other than a node."));

    QLabel *rightPanelClickedNodeOutDegreeLabel = new QLabel;
    rightPanelClickedNodeOutDegreeLabel -> setText (tr("Out-Degree:"));
    rightPanelClickedNodeOutDegreeLabel -> setToolTip (tr("The outDegree of a node is the sum of all outbound edge weights."));
    rightPanelClickedNodeOutDegreeLabel -> setStatusTip (tr("The outDegree of a node is the sum of all outbound edge weights."));
    rightPanelClickedNodeOutDegreeLCD=new QLCDNumber(5);
    rightPanelClickedNodeOutDegreeLCD -> setSegmentStyle(QLCDNumber::Flat);
    rightPanelClickedNodeOutDegreeLCD -> setStatusTip (tr("The sum of all outbound edge weights of the last clicked node. "
                                                          "Zero if you clicked something else."));
    rightPanelClickedNodeOutDegreeLCD -> setToolTip (tr("This is the sum of all outbound edge weights of the last clicked node. \n"
                                                        "Becomes zero when you click on something other than a node."));

    QLabel *rightPanelClickedNodeClucofLabel  = new QLabel;
    rightPanelClickedNodeClucofLabel -> setText (tr("Clu.Coef."));
    rightPanelClickedNodeClucofLabel -> setWhatsThis(
                tr("The Clustering Coefficient quantifies how close the clicked \n"
                   "vertex and its neighbors are to being a clique. \n"
                   "The value is the proportion of Edges between the vertices \n"
                   "within the neighbourhood of the clicked vertex, \n"
                   "divided by the number of Edges that could possibly exist "
                   "between them. \n\n"
                   "This value is automatically calculated only if vertices < 500.\n"
                   "If your network is larger than 500 vertices, compute CluCof "
                   "from the menu Analysis > Clustering Coefficient "));
    rightPanelClickedNodeClucofLabel -> setToolTip (
                tr("The Clustering Coefficient quantifies how close the clicked \n"
                   "vertex and its neighbors are to being a clique. \n\n"
                   "The value is the proportion of Edges between the vertices \n"
                   "within the neighbourhood of the clicked vertex, \n"
                   "divided by the number of Edges that could possibly exist \n"
                   "between them. \n\n"
                   "This value is automatically calculated only if vertices < 500.\n"
                   "If your network is larger than 500 vertices, compute CluCof "
                   "from the menu Analysis > Clustering Coefficient "));
    rightPanelClickedNodeClucofLCD = new QLCDNumber(5);
    rightPanelClickedNodeClucofLCD -> setSegmentStyle(QLCDNumber::Flat);
    rightPanelClickedNodeClucofLCD -> setStatusTip( tr("The Clustering Coefficient of the last clicked node. "
                                                       "Zero when you click on something else."));
    rightPanelClickedNodeClucofLCD -> setWhatsThis(
                tr("The Clustering Coefficient of the active node. \n"
                   "The Clustering Coefficient quantifies how close the clicked \n"
                       "vertex and its neighbors are to being a clique. \n"
                       "The value is the proportion of Edges between the vertices \n"
                       "within the neighbourhood of the clicked vertex, \n"
                       "divided by the number of Edges that could possibly exist "
                       "between them. \n\n"
                       "This value is automatically calculated only if vertices < 500.\n"
                       "If your network is larger than 500 vertices, compute CluCof "
                       "from the menu Analysis > Clustering Coefficient "));
    rightPanelClickedNodeClucofLCD  -> setToolTip (
                tr("The Clustering Coefficient of the active node. \n"
                   "The Clustering Coefficient quantifies how close the clicked \n"
                   "vertex and its neighbors are to being a clique. \n"
                   "The value is the proportion of Edges between the vertices \n"
                   "within the neighbourhood of the clicked vertex, \n"
                   "divided by the number of Edges that could possibly exist "
                   "between them. \n\n"
                   "This value is automatically calculated only if vertices < 500.\n"
                   "If your network is larger than 500 vertices, compute CluCof  \n"
                   "from the menu Analysis > Clustering Coefficient "));


    QLabel *verticalSpaceLabel3 = new QLabel;
    verticalSpaceLabel3-> setText ("");

    rightPanelClickedEdgeHeaderLabel = new QLabel;
    rightPanelClickedEdgeHeaderLabel-> setText (tr("Clicked Edge"));
    rightPanelClickedEdgeHeaderLabel->setFont(labelFont);

    QLabel *rightPanelClickedEdgeSourceLabel = new QLabel;
    rightPanelClickedEdgeSourceLabel -> setText (tr("Edge source:"));
    rightPanelClickedEdgeSourceLabel -> setToolTip (tr("The number of the last clicked edge source node."));
    rightPanelClickedEdgeSourceLCD =new QLCDNumber(5);
    rightPanelClickedEdgeSourceLCD ->setSegmentStyle(QLCDNumber::Flat);
    rightPanelClickedEdgeSourceLCD -> setToolTip (tr("This is the node number of the last clicked edge source node. \n"
                                                     "Becomes zero when you click on somethingto other than an edge"));
    rightPanelClickedEdgeSourceLCD -> setStatusTip (tr("The node number of the last clicked edge source node."
                                                       "Zero when you click on something else."));
    QLabel *rightPanelClickedEdgeTargetLabel = new QLabel;
    rightPanelClickedEdgeTargetLabel -> setText (tr("Edge target:"));
    rightPanelClickedEdgeTargetLabel -> setToolTip (tr("The number of the target node."));
    rightPanelClickedEdgeTargetLCD =new QLCDNumber(5);
    rightPanelClickedEdgeTargetLCD ->setSegmentStyle(QLCDNumber::Flat);
    rightPanelClickedEdgeTargetLCD -> setToolTip (tr("This is the node number of the last clicked edge target node. \n"
                                                     "Becomes zero when you click on something other than an edge"));
    rightPanelClickedEdgeTargetLCD -> setStatusTip (tr("The node number of the last clicked edge target node."
                                                       "Zero when you click on something else."));
    QLabel *rightPanelClickedEdgeWeightLabel = new QLabel;
    rightPanelClickedEdgeWeightLabel -> setText (tr("Edge weight:"));
    rightPanelClickedEdgeWeightLabel -> setToolTip (tr("The weight of the clicked edge."));
    rightPanelClickedEdgeWeightLCD =new QLCDNumber(5);
    rightPanelClickedEdgeWeightLCD ->setSegmentStyle(QLCDNumber::Flat);
    rightPanelClickedEdgeWeightLCD -> setToolTip (tr("This is the weight of the last clicked edge. \n"
                                                       "Becomes zero when you click on something other than an edge"));
    rightPanelClickedEdgeWeightLCD -> setStatusTip (tr("The weight of the last clicked edge. "
                                                         "Zero when you click on something else."));

    //create a grid layout
    QGridLayout *propertiesGrid = new QGridLayout();
    propertiesGrid -> setColumnMinimumWidth(0, 10);
    propertiesGrid -> setColumnMinimumWidth(1, 10);

    propertiesGrid -> addWidget(rightPanelNetworkTypeLabel , 0,0);
    propertiesGrid -> addWidget(rightPanelNodesLabel, 1,0);
    propertiesGrid -> addWidget(rightPanelNodesLCD,1,1);
    propertiesGrid -> addWidget(rightPanelEdgesLabel, 2,0);
    propertiesGrid -> addWidget(rightPanelEdgesLCD,2,1);
    propertiesGrid -> addWidget(rightPanelDensityLabel, 3,0);
    propertiesGrid -> addWidget(rightPanelDensityLCD,3,1);

    propertiesGrid -> addWidget(verticalSpaceLabel1, 4,0);

    propertiesGrid -> addWidget(rightPanelSelectedHeaderLabel, 5,0,1,2);
    propertiesGrid -> addWidget(rightPanelSelectedNodesLabel , 6,0);
    propertiesGrid -> addWidget(rightPanelSelectedNodesLCD ,6,1);
    propertiesGrid -> addWidget(rightPanelSelectedEdgesLabel, 7,0);
    propertiesGrid -> addWidget(rightPanelSelectedEdgesLCD, 7,1);

    propertiesGrid -> addWidget(verticalSpaceLabel2, 8,0);
    propertiesGrid -> addWidget(rightPanelClickedNodeHeaderLabel, 9,0,1,2);
    propertiesGrid -> addWidget(rightPanelClickedNodeLabel , 10,0);
    propertiesGrid -> addWidget(rightPanelClickedNodeLCD ,10,1);
    propertiesGrid -> addWidget(rightPanelClickedNodeInDegreeLabel, 11,0);
    propertiesGrid -> addWidget(rightPanelClickedNodeInDegreeLCD,11,1);
    propertiesGrid -> addWidget(rightPanelClickedNodeOutDegreeLabel, 12,0);
    propertiesGrid -> addWidget(rightPanelClickedNodeOutDegreeLCD,12,1);
    propertiesGrid -> addWidget(rightPanelClickedNodeClucofLabel, 13,0);
    propertiesGrid -> addWidget(rightPanelClickedNodeClucofLCD,13,1);

    propertiesGrid -> addWidget(verticalSpaceLabel3, 15,0);
    propertiesGrid -> addWidget(rightPanelClickedEdgeHeaderLabel, 16,0,1,2);
    propertiesGrid -> addWidget(rightPanelClickedEdgeSourceLabel , 17,0);
    propertiesGrid -> addWidget(rightPanelClickedEdgeSourceLCD ,17,1);
    propertiesGrid -> addWidget(rightPanelClickedEdgeTargetLabel , 18,0);
    propertiesGrid -> addWidget(rightPanelClickedEdgeTargetLCD ,18,1);
    propertiesGrid -> addWidget(rightPanelClickedEdgeWeightLabel , 19,0);
    propertiesGrid -> addWidget(rightPanelClickedEdgeWeightLCD ,19,1);


    propertiesGrid -> setRowStretch(20,1);   //make an invisible row stretch to rest of height

    //create a panel with title
    rightPanel = new QGroupBox(tr("Statistics Panel"));
    rightPanel->setMaximumWidth(210);
    rightPanel -> setLayout (propertiesGrid);

}







/**
 * @brief Initializes the scene and the corresponding graphicsWidget,
 * The latter is a QGraphicsView canvas which is the main widget of SocNetV.
 */
void MainWindow::initView() {
    qDebug ()<< "MW::initView()";
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
                                  " - To create a new node, double-click anywhere (Ctrl+.)\n"
                                  " - To add an arc between two nodes, double-click"
                                  " on the first node then double-click on the second (Ctrl+/)\n"
                                  " - To change network appearance, right click on empty space\n"
                                  " - To change/edit the properties of a node, right-click on it\n"
                                  " - To change/edit the properties of an edge, right-click on it."
                                  ""));

}




/**
 * @brief Initializes the application window UI:
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
    resetSlidersBtn->setStatusTip(tr("Reset zoom and rotation to zero (or press Ctrl+0)"));
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
 * @brief Connects signals & slots between various parts of the app:
 * - the GraphicsWidget and the Graph
 * - the GraphicsWidget and the MainWindow
 * This must be called after all widgets have been created.
 *
 */
void MainWindow::initSignalSlots() {
    qDebug ()<< "MW::initSignalSlots()";

    // Signals between graphicsWidget and MainWindow

    connect( graphicsWidget, SIGNAL( resized(int, int)),
                &activeGraph, SLOT( canvasSizeSet(int,int)) ) ;

    connect( graphicsWidget, &GraphicsWidget::setCursor,
                     this,&MainWindow::setCursor);

    connect( graphicsWidget,  &GraphicsWidget::userClickOnEmptySpace,
                     this, &MainWindow::slotEditClickOnEmptySpace ) ;

    connect( graphicsWidget, SIGNAL(
                 userDoubleClickNewNode(const QPointF &) ),
             this, SLOT(
                 slotEditNodeAddWithMouse(const QPointF &) ) ) ;

    connect( graphicsWidget, SIGNAL( userMiddleClicked(const int &, const int &) ),
             this, SLOT( slotEditEdgeCreate(const int &, const int &) ) 	);


    connect( graphicsWidget, SIGNAL( openNodeMenu() ),
             this, SLOT( slotEditNodeOpenContextMenu() ) ) ;

    connect( graphicsWidget, SIGNAL( openEdgeMenu() ),
             this, SLOT( slotEditEdgeOpenContextMenu() ) ) ;

    connect (graphicsWidget, &GraphicsWidget::openContextMenu,
             this, &MainWindow::slotEditOpenContextMenu);

    connect( graphicsWidget, SIGNAL(userNodeMoved(const int &, const int &, const int &)),
             this, SLOT( slotEditNodePosition(const int &, const int &, const int &) ) );

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


    // Signals between activeGraph and graphicsWidget

    connect( graphicsWidget, &GraphicsWidget::userSelectedItems,
                     &activeGraph,&Graph::graphSelectionChanged);

    connect( &activeGraph,
             SIGNAL( addGuideCircle(const double&, const double&, const double&) ),
             graphicsWidget,
             SLOT(  addGuideCircle(const double&, const double&, const double&) ) ) ;

    connect( &activeGraph, SIGNAL( addGuideHLine(const double&) ),
             graphicsWidget, SLOT(  addGuideHLine(const double&) ) ) ;

    connect( &activeGraph, SIGNAL( setNodePos(const int &, const qreal &, const qreal &) ),
             graphicsWidget, SLOT( moveNode(const int &, const qreal &, const qreal &) ) ) ;

    connect( &activeGraph,
             SIGNAL(
                 drawNode( const int &, const int &, const QString &,
                           const QString &,
                           const bool &,const bool &,
                           const QString &, const int &, const int &,
                           const bool &, const QString &,
                           const QString &, const int &, const int &,
                           const QPointF &
                            )
                 ),
             graphicsWidget,
             SLOT(
                 drawNode( const int &, const int &, const QString &,
                           const QString &,
                           const bool &,const bool &,
                           const QString &, const int &, const int &,
                           const bool &, const QString &,
                           const QString &, const int &, const int &,
                           const QPointF &
                            )
                 )
             ) ;

    connect( &activeGraph, SIGNAL( eraseEdge(const long int &, const long int &)),
             graphicsWidget, SLOT( eraseEdge(const long int &, const long int &) ) );


    connect( &activeGraph, SIGNAL( drawEdge( const int&, const int&, const float &,
                                             const QString &, const QString &,
                                             const int&, const bool&,
                                             const bool&,
                                             const bool&)),
             graphicsWidget, SLOT( drawEdge( const int&, const int&, const float &,
                                             const QString &,const QString &,
                                             const int &, const bool&,
                                             const bool &,
                                             const bool&) )  ) ;


    connect( &activeGraph, SIGNAL( setEdgeWeight(const long int &,
                                                   const long int &,
                                                   const float &)),
             graphicsWidget, SLOT( setEdgeWeight(const long int &,
                                                const long int &,
                                                const float &) ) );

    connect( &activeGraph, SIGNAL( setEdgeUndirected(const long int &,
                                                   const long int &,
                                                   const float &)),
             graphicsWidget, SLOT( setEdgeUndirected(const long int &,
                                                const long int &,
                                                const float &) ) );

    connect( &activeGraph, SIGNAL( setEdgeColor(const long int &,
                                                   const long int &,
                                                   const QString &)),
             graphicsWidget, SLOT( setEdgeColor(const long int &,
                                                const long int &,
                                                const QString &) ) );


    connect( &activeGraph, SIGNAL( setEdgeLabel(const long int &,
                                                   const long int &,
                                                   const QString &)),
             graphicsWidget, SLOT( setEdgeLabel(const long int &,
                                                const long int &,
                                                const QString &) ) );

    connect( &activeGraph, SIGNAL( eraseNode(long int) ),
             graphicsWidget, SLOT(  eraseNode(long int) ) );

    connect( &activeGraph, SIGNAL( setEdgeVisibility (int, int, int, bool) ),
             graphicsWidget, SLOT(  setEdgeVisibility (int, int, int, bool) ) );

    connect( &activeGraph, SIGNAL( setVertexVisibility(long int, bool)  ),
             graphicsWidget, SLOT(  setNodeVisibility (long int ,  bool) ) );

    connect( &activeGraph, SIGNAL( setNodeSize(const long int &, const int &)  ),
             graphicsWidget, SLOT(  setNodeSize (const long int &, const int &) ) );

    connect( &activeGraph, SIGNAL( setNodeColor(long int,QString))  ,
             graphicsWidget, SLOT(  setNodeColor(long int, QString) ) );

    connect( &activeGraph, SIGNAL( setNodeShape(long int,QString))  ,
             graphicsWidget, SLOT(  setNodeShape(long int, QString) ) );

    connect( &activeGraph, SIGNAL( setNodeNumberSize(const long int &, const int &)  ),
             graphicsWidget, SLOT(  setNodeNumberSize (const long int &, const int &) ) );

    connect( &activeGraph, SIGNAL( setNodeNumberDistance(const long int &, const int &)  ),
             graphicsWidget, SLOT( setNodeNumberDistance (const long int &, const int &) ) );

    connect( &activeGraph, &Graph::setNodeLabel ,
             graphicsWidget, &GraphicsWidget::setNodeLabel );

    connect( &activeGraph,&Graph::setNodeLabelColor,
             graphicsWidget,  &GraphicsWidget::setNodeLabelColor  );


    connect( &activeGraph, SIGNAL( setNodeLabelSize(const long int &, const int &)  ),
             graphicsWidget, SLOT(  setNodeLabelSize (const long int &, const int &) ) );

    connect( &activeGraph, SIGNAL( setNodeLabelDistance(const long int &, const int &)  ),
             graphicsWidget, SLOT( setNodeLabelDistance (const long int &, const int &) ) );


    connect( graphicsWidget, &GraphicsWidget::userClickedNode,
             &activeGraph, &Graph::vertexClickedSet );

    connect( graphicsWidget, &GraphicsWidget::userClickedEdge,
             &activeGraph, &Graph::edgeClickedSet );

    connect( &activeGraph, SIGNAL(signalRelationChangedToGW(int)),
             graphicsWidget, SLOT( relationSet(int))  ) ;


    //SIGNALS BETWEEN ACTIVEGRAPH AND MAINWINDOW

    connect( &activeGraph, &Graph::signalSelectionChanged,
                     this, &MainWindow::slotEditSelectionChanged);


    connect( &activeGraph, &Graph::signalNodeClickedInfo ,
             this, &MainWindow::slotEditNodeInfoStatusBar );

    connect ( &activeGraph, &Graph::signalEdgeClickedInfo,
             this, &MainWindow::slotEditEdgeInfoStatusBar );

    connect( &activeGraph,
             SIGNAL( signalGraphModified(const int &, const bool &,
                                  const int &, const int &,
                                  const float &) ),
             this,
             SLOT( slotNetworkChanged(const int &, const bool &,
                                      const int &, const int &,
                                      const float &) ) ) ;

    connect( &activeGraph, SIGNAL( signalGraphLoaded( const int &,
                                                      const QString &,
                                                      const QString &,
                                                      const int &,
                                                      const int &,
                                                      const QString &)
                                   ),
             this, SLOT( slotNetworkFileLoaded( const int &,
                                                const QString &,
                                                const QString &,
                                                const int &,
                                                const int &,
                                                const QString &)
                         )
             ) ;

    connect( &activeGraph, SIGNAL( signalGraphSaved( const int &) ),
             this, SLOT( slotNetworkSaved( const int &) ) ) ;

    connect( &activeGraph, SIGNAL( statusMessage (QString) ),
             this, SLOT( statusMessage (QString) ) ) ;

    connect( &activeGraph, SIGNAL( signalDatasetDescription (QString) ),
             this, SLOT( slotHelpMessageToUserInfo (QString) ) ) ;

    connect( editRelationChangeCombo , SIGNAL( activated(int) ) ,
             &activeGraph, SLOT( relationSet(int) ) );

    connect( editRelationChangeCombo , SIGNAL( currentTextChanged(QString) ),
             &activeGraph, SLOT( relationCurrentRename(QString) )  );

    connect( this , &MainWindow::signalRelationAddAndChange,
             &activeGraph, &Graph::relationAdd );

    connect( editRelationNextAct, &QAction::triggered,
             &activeGraph, &Graph::relationNext );

    connect( editRelationPreviousAct, &QAction::triggered,
             &activeGraph, &Graph::relationPrev );

    connect ( &activeGraph, &Graph::signalRelationChangedToMW,
                      this, &MainWindow::slotEditRelationChange );

    connect ( &activeGraph, &Graph::signalRelationsClear,
              this, &MainWindow::slotEditRelationsClear );

    connect ( &activeGraph, &Graph::signalRelationAddToMW,
              this, &MainWindow::slotEditRelationAdd  );

    connect ( &activeGraph, &Graph::signalRelationRenamedToMW,
                      this, &MainWindow::slotEditRelationRename );


    connect ( &activeGraph, &Graph::signalProgressBoxCreate,
              this, &MainWindow::slotProgressBoxCreate);

    connect ( &activeGraph, &Graph::signalProgressBoxKill,
              this, &MainWindow::slotProgressBoxDestroy);



    //signals and slots inside MainWindow


    connect( editRelationAddAct, SIGNAL(triggered()),
             this, SLOT(slotEditRelationAdd()) );

    connect( editRelationRenameAct,SIGNAL(triggered()),
             this, SLOT(slotEditRelationRename()) ) ;


    connect( &m_DialogEdgeFilterByWeight, SIGNAL( userChoices( float, bool) ),
             &activeGraph, SLOT( edgeFilterByWeight (float, bool) ) );


    connect( &m_WebCrawlerDialog, &WebCrawlerDialog::userChoices,
             this, &MainWindow::slotNetworkWebCrawler );

    connect( &m_datasetSelectDialog, SIGNAL( userChoices( QString) ),
             this, SLOT( slotNetworkDataSetRecreate(QString) ) );


    connect( layoutGuidesAct, SIGNAL(triggered(bool)),
             this, SLOT(slotLayoutGuides(bool)));


    connect(toolBoxEditNodeSubgraphSelect, SIGNAL (currentIndexChanged(int) ),
            this, SLOT(toolBoxEditNodeSubgraphSelectChanged(int) ) );


    connect(toolBoxEditEdgeModeSelect, SIGNAL (currentIndexChanged(int) ),
                this, SLOT(slotEditEdgeMode(int) ) );

    connect(toolBoxEditEdgeSymmetrizeSelect, SIGNAL (currentIndexChanged(int) ),
            this, SLOT(toolBoxEditEdgeSymmetrizeSelectChanged(int) ) );

    connect(toolBoxAnalysisMatricesSelect, SIGNAL (currentIndexChanged(int) ),
        this, SLOT(toolBoxAnalysisMatricesSelectChanged(int) ) );

    connect(toolBoxAnalysisCohesionSelect, SIGNAL (currentIndexChanged(int) ),
            this, SLOT(toolBoxAnalysisCohesionSelectChanged(int) ) );

    connect(toolBoxAnalysisStrEquivalenceSelect, SIGNAL (currentIndexChanged(int) ),
            this, SLOT(toolBoxAnalysisStrEquivalenceSelectChanged(int) ) );

    connect(toolBoxAnalysisCommunitiesSelect, SIGNAL (currentIndexChanged(int) ),
            this, SLOT(toolBoxAnalysisCommunitiesSelectChanged(int) ) );

    connect(toolBoxAnalysisProminenceSelect, SIGNAL (currentIndexChanged(int) ),
            this, SLOT(toolBoxAnalysisProminenceSelectChanged(int) ) );


    connect(toolBoxLayoutByIndexApplyButton, SIGNAL (clicked() ),
            this, SLOT(toolBoxLayoutByIndexApplyBtnPressed() ) );

    connect(toolBoxLayoutForceDirectedApplyButton, SIGNAL (clicked() ),
            this, SLOT(toolBoxLayoutForceDirectedApplyBtnPressed() ) );


}







/**
 * @brief Initializes the default network parameters.
 * Used on app start and especially when erasing a network to start a new one
 */
void MainWindow::initApp(){
    qDebug()<<"MW::initApp() - START INITIALIZATION";

    statusMessage( tr("Application initialization. Please wait..."));

    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

    // Init basic variables

    considerWeights=false;
    inverseWeights=false;
    askedAboutWeights=false;

    previous_fileName=fileName;
    fileName="";

    initFileCodec= "UTF-8";

    networkSave->setIcon(QIcon(":/images/saved.png"));
    networkSave->setEnabled(true);

    markedNodesExist=false;	//used by slotEditNodeFind()


    /** Clear LCDs **/
    rightPanelClickedNodeInDegreeLCD->display(0);
    rightPanelClickedNodeOutDegreeLCD->display(0);
    rightPanelClickedNodeClucofLCD->display(0);
    rightPanelClickedNodeLCD->display(0);
    rightPanelClickedEdgeSourceLCD->display(0);
    rightPanelClickedEdgeTargetLCD->display(0);
    rightPanelClickedEdgeWeightLCD->display(0);

    /** Clear toolbox and menu checkboxes **/
    toolBoxEditEdgeSymmetrizeSelect->setCurrentIndex(0);
    toolBoxEditEdgeModeSelect->setCurrentIndex(0);
    toolBoxAnalysisCommunitiesSelect->setCurrentIndex(0);
    toolBoxAnalysisStrEquivalenceSelect->setCurrentIndex(0);
    toolBoxAnalysisCohesionSelect->setCurrentIndex(0);
    toolBoxAnalysisProminenceSelect->setCurrentIndex(0);
    toolBoxLayoutByIndexSelect->setCurrentIndex(0);
    toolBoxLayoutByIndexTypeSelect ->setCurrentIndex(0);
    toolBoxLayoutForceDirectedSelect->setCurrentIndex(0);

    optionsEdgeWeightNumbersAct->setChecked(
                (appSettings["initEdgeWeightNumbersVisibility"] == "true") ? true:false
                );
    considerEdgeWeightsAct->setChecked(false);
    optionsEdgeArrowsAct->setChecked(
                (appSettings["initEdgeArrows"] == "true") ? true: false
            );

    optionsEdgeLabelsAct->setChecked (
                (appSettings["initEdgeLabelsVisibility"] == "true") ? true: false
                );
    editFilterNodesIsolatesAct->setChecked(false); // re-init orphan nodes menu item

    editFilterEdgesUnilateralAct->setChecked(false);

    //editRelationChangeCombo->clear();


    /** Clear previous network data */
    activeGraph.clear();
    activeGraph.setSocNetV_Version(VERSION);

    activeGraph.vertexShapeInit(appSettings["initNodeShape"]);
    activeGraph.vertexSizeInit(appSettings["initNodeSize"].toInt(0, 10));
    activeGraph.vertexColorInit( appSettings["initNodeColor"] );

    activeGraph.vertexNumberSizeInit(appSettings["initNodeNumberSize"].toInt(0,10));
    activeGraph.vertexNumberColorInit(appSettings["initNodeNumberColor"]);
    activeGraph.vertexNumberDistanceInit(appSettings["initNodeNumberDistance"].toInt(0,10));

    activeGraph.vertexLabelColorInit(appSettings["initNodeLabelColor"]);
    activeGraph.vertexLabelSizeInit(appSettings["initNodeLabelSize"].toInt(0,10));
    activeGraph.vertexLabelDistanceInit(appSettings["initNodeLabelDistance"].toInt(0,10));

    activeGraph.edgeColorInit(appSettings["initEdgeColor"]);

    activeGraph.vertexLabelsVisibilitySet(
                (appSettings["initNodeLabelsVisibility"] == "true" ) ? true: false
                );
    activeGraph.vertexNumbersVisibilitySet(
                ( appSettings["initNodeNumbersVisibility"] == "true" ) ? true: false
                );
    activeGraph.vertexNumbersInsideNodesSet(
                ( appSettings["initNodeNumbersInside"] == "true" ) ? true: false
                );

    /** Clear graphicsWidget scene and reset transformations **/
    graphicsWidget->clear();
    rotateSlider->setValue(0);
    zoomSlider->setValue(250);
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
                    );
    }

    qDebug()<<"MW::initApp() - Clearing my"
           <<m_textEditors.size()
          <<"textEditors";
    foreach ( TextEditor *ed, m_textEditors) {
        ed->close();
        delete ed;
    }
    m_textEditors.clear();

    /** set window title **/
    setWindowTitle(tr("Social Network Visualizer ")+VERSION);

    QApplication::restoreOverrideCursor();

    setCursor(Qt::ArrowCursor);

    statusMessage( tr("Ready"));
    qDebug()<< "MW::initApp() - INITIALISATION END";


}



/**
 * @brief Updates the Recent Files QActions in the menu
 */
void MainWindow::slotNetworkFileRecentUpdateActions() {

    int numRecentFiles = qMin(recentFiles.size(), (int)MaxRecentFiles);

    for (int i = 0; i < numRecentFiles; ++i) {
        QString text = tr("&%1  %2").arg(i + 1).arg(QFileInfo(recentFiles[i]).fileName());
        recentFileActs[i]->setText(text);
        recentFileActs[i]->setData(recentFiles[i]);
        recentFileActs[i]->setVisible(true);
    }
    for (int j = numRecentFiles; j < MaxRecentFiles; ++j)
        recentFileActs[j]->setVisible(false);

    //separatorAct->setVisible(numRecentFiles > 0);
}



/**
 * @brief  Convenience method to show a message in the status bar, with the given duration
 * Slot called by Graph::statusMessage to display some message to the user
 * @param message
 */
void MainWindow::statusMessage(const QString message){
    statusBar()->showMessage( message, appSettings["initStatusBarDuration"].toInt(0));
}



/**
 * @brief Helper function to display a useful info message
 * @param text
 */
void MainWindow::slotHelpMessageToUserInfo(const QString text) {
    slotHelpMessageToUser(USER_MSG_INFO,tr("Useful information"), text  );
}


/**
 * @brief Helper function to display a useful error message
 * @param text
 */
void MainWindow::slotHelpMessageToUserError(const QString text) {
    slotHelpMessageToUser(USER_MSG_CRITICAL ,tr("Error"), text  );
}


/**
 * @brief Convenience method
 * @param message
 */
int MainWindow::slotHelpMessageToUser(const int type,
                                      const QString statusMsg,
                                      const QString text,
                                      const QString info,
                                      QMessageBox::StandardButtons buttons,
                                      QMessageBox::StandardButton defBtn,
                                      const QString btn1,
                                      const QString btn2
                                      ) {
    int response=0;
    QMessageBox msgBox;
    QPushButton *pbtn1, *pbtn2;

    switch (type) {
    case USER_MSG_INFO:
        if (!statusMsg.isNull()) statusMessage(  statusMsg  );

        msgBox.setText(text);
        if (!info.isNull()) msgBox.setInformativeText(info);
        msgBox.setIcon(QMessageBox::Information);
        if (buttons==QMessageBox::NoButton) {
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setDefaultButton(QMessageBox::Ok);
        }
        else {
            msgBox.setStandardButtons(buttons);
            msgBox.setDefaultButton(defBtn);
        }
        msgBox.setDefaultButton(defBtn);
        response = msgBox.exec();

        break;

    case USER_MSG_CRITICAL:
        if (!statusMsg.isNull()) statusMessage(  statusMsg  );

        msgBox.setText(text);
        if (!info.isNull()) msgBox.setInformativeText(info);
        //msgBox.setTextFormat(Qt::RichText);
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        response = msgBox.exec();

        break;

    case USER_MSG_CRITICAL_NO_NETWORK:
        statusMessage(  tr("Nothing to do! Load or create a social network first")  );

        msgBox.setText(
                    tr("No network! \n"
                       "Load social network data or create a new social network first. \n")
                    );
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        response = msgBox.exec();

        break;

    case USER_MSG_CRITICAL_NO_EDGES:
        statusMessage(  tr("Nothing to do! Load social network data or create edges first")  );

        msgBox.setText(
                    tr("No edges! \n"
                       "Load social network data or create some edges first. \n")
                    );
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        response = msgBox.exec();

        break;

    case USER_MSG_QUESTION:
        if (!statusMsg.isNull()) statusMessage(  statusMsg  );

        msgBox.setText( text );
        if (!info.isNull()) msgBox.setInformativeText(info);
        if (buttons==QMessageBox::NoButton) {
            msgBox.setStandardButtons(QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel);
            msgBox.setDefaultButton(QMessageBox::Yes);
        }
        else {
            msgBox.setStandardButtons(buttons);
            msgBox.setDefaultButton(defBtn);
        }

        msgBox.setIcon(QMessageBox::Question);
        response = msgBox.exec();

        break;

    case USER_MSG_QUESTION_CUSTOM: // a custom question with just two buttons
        if (!statusMsg.isNull()) statusMessage(  statusMsg  );

        msgBox.setText( text );
        if (!info.isNull()) msgBox.setInformativeText(info);
        pbtn1 = msgBox.addButton(btn1, QMessageBox::ActionRole);
        pbtn2 = msgBox.addButton(btn2, QMessageBox::ActionRole);
        msgBox.setIcon(QMessageBox::Question);
        response = msgBox.exec();
        if (msgBox.clickedButton() == pbtn1 ) {
            response=1;
        }
        else if (msgBox.clickedButton() == pbtn2 ) {
            response=2;
        }
        break;
    default: //just for sanity
        if (!statusMsg.isNull()) statusMessage(  statusMsg  );
        msgBox.setText( text );
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        response = msgBox.exec();
        break;
    }
    return response;

}






/**
 * @brief Called from MW, when user selects something in the Subgraph from Selected
 * Nodes selectbox of the toolbox
 * @param selectedIndex
 */
void MainWindow::toolBoxEditNodeSubgraphSelectChanged(int selectedIndex) {
    qDebug()<< "MW::toolBoxEditNodeSubgraphSelectChanged "
               "selected text index: " << selectedIndex;
    switch(selectedIndex){
    case 0:
        break;
    case 1:
        slotEditNodeSelectedToClique();
        break;
    case 2:
        slotEditNodeSelectedToStar();
        break;
    case 3:
        slotEditNodeSelectedToCycle();
        break;
    case 4:
        slotEditNodeSelectedToLine();
        break;
    };
}





/**
 * @brief Called from MW, when user selects something in the Edge Symmetrize
 * selectbox of the toolbox
 * @param selectedIndex
 */
void MainWindow::toolBoxEditEdgeSymmetrizeSelectChanged(int selectedIndex) {
    qDebug()<< "MW::toolBoxEditEdgeSymmetrizeSelectChanged "
               "selected text index: " << selectedIndex;
    switch(selectedIndex){
    case 0:
        break;
    case 1:
        slotEditEdgeSymmetrizeAll();
        break;
    case 2:
        slotEditEdgeSymmetrizeStrongTies();
        break;
    case 3:
        slotEditEdgeSymmetrizeCocitation();
        break;
    };
}




/**
 * @brief Called from MW, when user selects something in the Matrices
 * selectbox of the toolbox
 * @param selectedIndex
 */
void MainWindow::toolBoxAnalysisMatricesSelectChanged(int selectedIndex) {
    qDebug()<< "MW::toolBoxAnalysisMatricesSelectChanged "
               "selected text index: " << selectedIndex;
    switch(selectedIndex){
    case 0:
        break;
    case 1:
        slotNetworkViewSociomatrix();
        break;
    case 2:
        slotNetworkViewSociomatrixPlotText();
        break;
    case 3:
        slotAnalyzeMatrixAdjacencyInverse();
        break;
    case 4:
        slotAnalyzeMatrixAdjacencyTranspose();
        break;
    case 5:
        slotAnalyzeMatrixAdjacencyCocitation();
        break;
    case 6:
        slotAnalyzeMatrixDegree();
        break;
    case 7:
        slotAnalyzeMatrixLaplacian();
        break;
    };
}




/**
 * @brief Called from MW, when user selects something in the Cohesion
 * selectbox of the toolbox to compute basic graph theoretic / network properties
 * @param selectedIndex
 */
void MainWindow::toolBoxAnalysisCohesionSelectChanged(int selectedIndex) {
    qDebug()<< "MW::toolBoxAnalysisCohesionSelectChanged "
               "selected text index: " << selectedIndex;
    switch(selectedIndex){
    case 0:
        break;
    case 1:
        slotAnalyzeReciprocity();
        break;
    case 2:
        slotAnalyzeSymmetryCheck();
        break;
    case 3:
        slotAnalyzeDistance();
        break;
    case 4:
        slotAnalyzeDistanceAverage();
        break;
    case 5:
        slotAnalyzeMatrixDistances();
        break;
    case 6:
        slotAnalyzeMatrixGeodesics();
        break;
    case 7:
        slotAnalyzeEccentricity();
        break;
    case 8:
        slotAnalyzeDiameter();
        break;
    case 9:
        slotAnalyzeConnectedness();
        break;
    case 10:
        slotAnalyzeWalksLength();
        break;
    case 11:
        slotAnalyzeWalksTotal();
        break;
    case 12:
        slotAnalyzeReachabilityMatrix();
        break;
    case 13:
        slotAnalyzeClusteringCoefficient();
        break;
    };


}






/**
 * @brief Called from MW, when user selects something in the Communities selectbox
 * of the toolbox
 * @param selectedIndex
 *
 */
void MainWindow::toolBoxAnalysisCommunitiesSelectChanged(int selectedIndex) {
    qDebug()<< "MW::toolBoxAnalysisCommunitiesSelectChanged "
               "selected text index: " << selectedIndex;
    switch(selectedIndex){
    case 0:
        break;
    case 1:
        qDebug()<< "Cliques";
        slotAnalyzeCommunitiesCliqueCensus();
        break;
    case 2:
        qDebug() << "Triad Census";
        slotAnalyzeCommunitiesTriadCensus();
        break;
    };

}





/**
 * @brief Called from MW, when user selects something in the Structural Equivalence
 * selectbox of the toolbox
 * @param selectedIndex
 *
 */
void MainWindow::toolBoxAnalysisStrEquivalenceSelectChanged(int selectedIndex) {
    qDebug()<< "MW::toolBoxAnalysisStrEquivalenceSelectChanged "
               "selected text index: " << selectedIndex;
    switch(selectedIndex){
    case 0:
        break;
    case 1:
        qDebug()<< "Pearson";
        slotAnalyzeStrEquivalencePearsonDialog();
        break;
    case 2:
        qDebug()<< "Similarities";
        slotAnalyzeStrEquivalenceSimilarityMeasureDialog();
        break;
    case 3:
        qDebug() << "Dissimilarities";
        slotAnalyzeStrEquivalenceDissimilaritiesDialog();
        break;
    case 4:
        qDebug() << "Hierarchical Clustering";
        slotAnalyzeStrEquivalenceClusteringHierarchicalDialog();
        break;
    };

}





/**
 * @brief Called from MW, when user selects something in the Prominence selectbox
 *  of the toolbox
 * @param selectedIndex
 *
 */
void MainWindow::toolBoxAnalysisProminenceSelectChanged(int selectedIndex) {
    qDebug()<< "MW::toolBoxAnalysisProminenceSelectChanged "
               "selected text index: " << selectedIndex;
    switch(selectedIndex){
    case 0:
        break;
    case 1:
        slotAnalyzeCentralityDegree();
        break;
    case 2:
        slotAnalyzeCentralityCloseness();
        break;
    case 3:
        slotAnalyzeCentralityClosenessIR();
        break;
    case 4:
        slotAnalyzeCentralityBetweenness();
        break;
    case 5:
        slotAnalyzeCentralityStress();
        break;
    case 6:
        slotAnalyzeCentralityEccentricity();
        break;
    case 7:
        slotAnalyzeCentralityPower();
        break;
    case 8:
        slotAnalyzeCentralityInformation();
        break;
    case 9:
        slotAnalyzeCentralityEigenvector();
        break;
    case 10:
        slotAnalyzePrestigeDegree();
        break;
    case 11:
        slotAnalyzePrestigePageRank();
        break;
    case 12:
        slotAnalyzePrestigeProximity();
        break;
    };

}

/**
 * @brief Called from MW, when user selects a Prominence index in the Layout selectbox
 *  of the Control Panel .
 */
void MainWindow::toolBoxLayoutByIndexApplyBtnPressed(){
    qDebug()<<"MW::toolBoxLayoutByIndexApplyBtnPressed()";
    int selectedIndex = toolBoxLayoutByIndexSelect->currentIndex();
    QString selectedIndexText = toolBoxLayoutByIndexSelect -> currentText();
    int selectedLayoutType = toolBoxLayoutByIndexTypeSelect ->currentIndex();
    qDebug()<<"MW::toolBoxLayoutByIndexApplyBtnPressed() - selected index is "
             << selectedIndexText << " : " << selectedIndex
             << " selected layout type is " << selectedLayoutType;
    switch(selectedIndex) {
    case 0:
        break;
    case 1:
        if (selectedLayoutType==0)
            slotLayoutRadialRandom();
        else if (selectedLayoutType==1)
            slotLayoutRandom();
        break;
    default:
        if (selectedLayoutType==0)  { // radial
            slotLayoutRadialByProminenceIndex(selectedIndexText);
        }
        else if (selectedLayoutType==1)  { // on levels
            slotLayoutLevelByProminenceIndex(selectedIndexText);
        }
        else if (selectedLayoutType==2) { //  node size
            slotLayoutNodeSizeByProminenceIndex(selectedIndexText);
            // re-init other options for node sizes...
        }
        else if (selectedLayoutType==3){  // node color
            slotLayoutNodeColorByProminenceIndex(selectedIndexText);
        }
        break;
    };
}



/**
 * @brief Called from MW, when user selects a model in the Layout by Force Directed
 * selectbox of left panel.
 */
void MainWindow::toolBoxLayoutForceDirectedApplyBtnPressed(){
    qDebug()<<"MW::toolBoxLayoutForceDirectedApplyBtnPressed()";
    int selectedModel = toolBoxLayoutForceDirectedSelect->currentIndex();
    QString selectedModelText = toolBoxLayoutForceDirectedSelect -> currentText();
    qDebug() << " selected index is " << selectedModelText << " : "
             << selectedModel;

    switch(selectedModel) {
    case 0:
        break;
    case 1:
        slotLayoutGuides(false);
        slotLayoutKamadaKawai();
        break;
    case 2:
        slotLayoutGuides(false);
        slotLayoutFruchterman();
        break;
    case 3:
        slotLayoutGuides(false);
        slotLayoutSpringEmbedder();
        break;
    default:
        toolBoxLayoutForceDirectedSelect->setCurrentIndex(0);
        break;
    };
}






/**
 * @brief Resizes the scene when the window is resized.
 */
void MainWindow::resizeEvent( QResizeEvent * ){

    qDebug() << "MW::resizeEvent():  Window resized to"
             << width()
             << ","
             << height()
             <<"Calling activeGraph.canvasSizeSet() to set canvas width and height";

    activeGraph.canvasSizeSet(graphicsWidget->width(),graphicsWidget->height());

    statusMessage(
                QString(
                    tr("Window resized to (%1, %2)px. Canvas size: (%3, %4) px"))
                .arg(width()).arg(height())
                .arg(graphicsWidget->width()).arg(graphicsWidget->height())
                );

}




/**
 * @brief Called when the application closes. Asks to write any unsaved network data.
 * @param ce
 */
void MainWindow::closeEvent( QCloseEvent* ce ) {
    qDebug() << "MW::closeEvent()";
    if ( activeGraph.graphSaved()  )  {
        ce->accept();
        return;
    }

    switch( slotHelpMessageToUser(
                USER_MSG_QUESTION,
                tr("Save changes"),
                tr("Modified network has not been saved!"),
                tr("Do you want to save the changes to the network file?"),
                 QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel, QMessageBox::Cancel
                ) )
    {
    case QMessageBox::Yes:
        slotNetworkSave();
        ce->accept();
        break;
    case QMessageBox::No:
        ce->accept();
        break;
    case QMessageBox::NoButton:
    default: // just for sanity
        ce->ignore();
        break;
    }

    initApp();
}



/**
 * @brief Creates a new network
 */
void MainWindow::slotNetworkNew() {
    slotNetworkClose();
}



/**
 * @brief Returns the last path used by user to open/save something
 */
QString MainWindow::getLastPath() {
    if ( appSettings["lastUsedDirPath"] == "socnetv-initial-none") {
        appSettings["lastUsedDirPath"] = appSettings["dataDir"];
    }
    qDebug()<< "MW::getLastPath()" << appSettings["lastUsedDirPath"] ;
    return appSettings["lastUsedDirPath"] ;
}


/**
 * @brief Sets the last path used by user to open/save something
 * @param filePath
 */
void MainWindow::setLastPath(QString filePath) {
    qDebug()<< "MW::setLastPath() for " << filePath;
    appSettings["lastUsedDirPath"] = QFileInfo(filePath).dir().absolutePath();

    if (    !QFileInfo(filePath).completeSuffix().toLower().contains( "bmp" ) &&
            !QFileInfo(filePath).completeSuffix().toLower().contains( "jpg" ) &&
            !QFileInfo(filePath).completeSuffix().toLower().contains( "png" ) &&
            !QFileInfo(filePath).completeSuffix().toLower().contains( "pdf" )
            ) {
        recentFiles.removeAll(filePath);
        recentFiles.prepend(filePath);
        while(recentFiles.size() > MaxRecentFiles )
            recentFiles.removeLast();
    }
    slotNetworkFileRecentUpdateActions();
    saveSettings();

    qDebug() << appSettings["lastUsedDirPath"];
}



/**
 * @brief If m_fileName is empty, opens a file selection dialog
 * Then calls slotNetworkFilePreview()
 * Called on application loading from command line with filename parameter
 * Called from slotNetworkImport* methods
 * Called from slotNetworkFileLoadRecent
 * @param m_fileName
 * @param m_fileFormat
 * @param checkSelectFileType
 */
void MainWindow::slotNetworkFileChoose(QString m_fileName,
                                       int m_fileFormat,
                                       const bool &checkSelectFileType) {
    qDebug() << "MW::slotNetworkFileChoose() - "
             << " m_fileName: " << m_fileName
             << " m_fileFormat " << m_fileFormat
             << " checkSelectFileType " << checkSelectFileType;

    previous_fileName=fileName;
    QString fileType_filter;

    /*
     * CASE 1: No filename provided. This happens when:
     * - User clicked Open Network File or
     * - User clicked Import Network
     *
     * Prepare known filetypes and
     * Open a file selection dialog for the user
     *
     */
    if (m_fileName.isNull() || m_fileName.isEmpty() ) {

        fileType=m_fileFormat;

        // prepare supported filetype extensions
        switch (fileType){
        case FILE_GRAPHML:
            fileType_filter = tr("GraphML (*.graphml *.xml);;All (*)");
            break;
        case FILE_PAJEK:
            fileType_filter = tr("Pajek (*.net *.paj *.pajek);;All (*)");
            break;
        case FILE_ADJACENCY:
            fileType_filter = tr("Adjacency (*.csv *.sm *.adj *.txt);;All (*)");
            break;
        case FILE_GRAPHVIZ:
            fileType_filter = tr("GraphViz (*.dot);;All (*)");
            break;
        case FILE_UCINET:
            fileType_filter = tr("UCINET (*.dl *.dat);;All (*)");
            break;
        case FILE_GML:
            fileType_filter = tr("GML (*.gml);;All (*)");
            break;

        case FILE_EDGELIST_WEIGHTED:
            fileType_filter = tr("Weighted Edge List (*.csv *.txt *.list *.edgelist *.lst *.wlst);;All (*)");
            break;
        case FILE_EDGELIST_SIMPLE:
            fileType_filter = tr("Simple Edge List (*.csv *.txt *.list *.edgelist *.lst);;All (*)");
            break;
        case FILE_TWOMODE:
            fileType_filter = tr("Two-Mode Sociomatrix (*.2sm *.aff);;All (*)");
            break;
        default:	//All
            fileType_filter = tr("GraphML (*.graphml *.xml);;"
                                 "GML (*.gml *.xml);;"
                                 "Pajek (*.net *.pajek *.paj);;"
                                 "UCINET (*.dl *.dat);;"
                                 "Adjacency (*.csv *.adj *.sm *.txt);;"
                                 "GraphViz (*.dot);;"
                                 "Weighted Edge List (*.csv *.txt *.edgelist *.list *.lst *.wlst);;"
                                 "Simple Edge List (*.csv *.txt *.edgelist *.list *.lst);;"
                                 "Two-Mode Sociomatrix (*.2sm *.aff);;"
                                  "All (*)");
            break;

        }
        //prepare the filedialog
        QFileDialog *fileDialog = new QFileDialog(this);
        fileDialog->setFileMode(QFileDialog::ExistingFile);
        fileDialog->setNameFilter(fileType_filter);
        fileDialog->setViewMode(QFileDialog::Detail);
        fileDialog->setDirectory(getLastPath());

        //connect its signals to our slots
        connect ( fileDialog, &QFileDialog::filterSelected,
                  this, &MainWindow::slotNetworkFileDialogFilterSelected);
        connect ( fileDialog, &QFileDialog::fileSelected,
                  this, &MainWindow::slotNetworkFileDialogFileSelected);
        connect ( fileDialog, &QFileDialog::rejected ,
                  this, &MainWindow::slotNetworkFileDialogRejected);

        //open the filedialog
        statusMessage( tr("Choose a network file..."));
        if (fileDialog->exec()) {
            m_fileName = (fileDialog->selectedFiles()).at(0);
            qDebug() << "MW::slotNetworkFileChoose() - m_fileName " << m_fileName;

        }
        else {
            //display some error
            statusMessage( tr("Nothing to do..."));
        }
        return;

    }


    /*
     * CASE 2: Filename provided. This happens when:
     * - Application starts from command line with filename parameter or
     * - User selects a Recent File or
     * - User selects a file in a previous slotNetworkFileChoose call
     *
     * If checkSelectFileType==TRUE (that is on app start or Recent File),
     * it tries to understand fileType by file extension. If file has unknown
     * file extension or an ambiguous file extension used by many different file
     * formats, then it asks the user to provide the fileType. Then it loads the
     * file
     *
     * If checkSelectFileType==FALSE, then it loads the file with given fileType.
     *
     */
    if (checkSelectFileType || m_fileFormat==FILE_UNRECOGNIZED) {
        // This happens only on application startup or on loading a recent file.
        if ( ! m_fileName.endsWith(".graphml",Qt::CaseInsensitive ) &&
             ! m_fileName.endsWith(".net",Qt::CaseInsensitive ) &&
             ! m_fileName.endsWith(".paj",Qt::CaseInsensitive )  &&
             ! m_fileName.endsWith(".pajek",Qt::CaseInsensitive ) &&
             ! m_fileName.endsWith(".dl",Qt::CaseInsensitive ) &&
             ! m_fileName.endsWith(".gml",Qt::CaseInsensitive ) &&
             ! m_fileName.endsWith(".wlst",Qt::CaseInsensitive ) &&
             ! m_fileName.endsWith(".wlist",Qt::CaseInsensitive )&&
             ! m_fileName.endsWith(".2sm",Qt::CaseInsensitive ) &&
             ! m_fileName.endsWith(".aff",Qt::CaseInsensitive )) {
            //ambigious

            tempFileNameNoPath=m_fileName.split ("/");
            QStringList fileTypes;
            fileTypes << tr("GraphML")
                      << tr("GML")
                      << tr("Pajek")
                      << tr("UCINET")
                      << tr("Adjacency")
                      << tr("GraphViz")
                      << tr("Edge List (weighted)")
                      << tr("Edge List (simple, non-weighted)")
                      << tr("Two-mode sociomatrix") ;

            bool ok;
            QString userFileType= QInputDialog::getItem(
                        this,
                        tr("Selected file has ambiguous file extension!"),
                        tr("You selected: %1 \n"

                           "The name of this file has either an unknown extension \n"
                           "or an extension used by different network file formats.\n\n"

                           "SocNetV supports the following social network file "
                           "formats. \nIn parentheses the expected extension. \n"
                        "- GraphML (.graphml or .xml)\n"
                           "- GML (.gml or .xml)\n"
                        "- Pajek (.paj or .pajek or .net)\n"
                        "- UCINET (.dl .dat) \n"
                        "- GraphViz (.dot)\n"
                        "- Adjacency Matrix (.sm or .adj or .csv or .txt)\n"
                        "- Simple Edge List (.list or .lst)\n"
                        "- Weighted Edge List (.wlist or .wlst)\n"
                        "- Two-Mode / affiliation (.2sm or .aff) \n\n"

                        "If you are sure the file is of a supported format, please \n"
                           "select the right format from the list below.").
                        arg(tempFileNameNoPath.last()),
                        fileTypes, 0, false, &ok);
            if (ok && !userFileType.isEmpty()) {
                if (userFileType == "GraphML") {
                    m_fileFormat=FILE_GRAPHML;
                }
                else if (userFileType == "GraphML") {
                    m_fileFormat=FILE_PAJEK;
                }
                else if (userFileType == "GML") {
                    m_fileFormat=FILE_GML;
                }
                else if (userFileType == "UCINET") {
                    m_fileFormat=FILE_UCINET;
                }
                else if (userFileType == "Adjacency") {
                    m_fileFormat=FILE_ADJACENCY;
                }
                else if (userFileType == "GraphViz") {
                    m_fileFormat=FILE_GRAPHVIZ;
                }
                else if (userFileType == "Edge List (weighted)") {
                    m_fileFormat=FILE_EDGELIST_WEIGHTED;
                }
                else if (userFileType == "Edge List (simple, non-weighted)") {
                    m_fileFormat=FILE_EDGELIST_SIMPLE;
                }
                else if (userFileType == "Two-mode sociomatrix") {
                    m_fileFormat=FILE_TWOMODE;
                }

            }
            else {
                statusMessage( tr("Opening network file aborted."));
                //if a file was previously opened, get back to it.
                if (activeGraph.graphLoaded())	{
                    fileName=previous_fileName;
                }
                return;
            }

        }

        else if (m_fileName.endsWith(".graphml",Qt::CaseInsensitive ) ||
                m_fileName.endsWith(".xml",Qt::CaseInsensitive ) ) {
            m_fileFormat=FILE_GRAPHML;
        }
        else if (m_fileName.endsWith(".net",Qt::CaseInsensitive ) ||
                 m_fileName.endsWith(".paj",Qt::CaseInsensitive )  ||
                 m_fileName.endsWith(".pajek",Qt::CaseInsensitive ) ) {
            m_fileFormat=FILE_PAJEK;
        }
        else if (m_fileName.endsWith(".dl",Qt::CaseInsensitive ) ||
                 m_fileName.endsWith(".dat",Qt::CaseInsensitive ) ) {
            m_fileFormat=FILE_UCINET;
        }
        else if (m_fileName.endsWith(".sm",Qt::CaseInsensitive ) ||
                 m_fileName.endsWith(".csv",Qt::CaseInsensitive ) ||
                 m_fileName.endsWith(".adj",Qt::CaseInsensitive ) ||
                 m_fileName.endsWith(".txt",Qt::CaseInsensitive )) {
            m_fileFormat=FILE_ADJACENCY;
        }
        else if (m_fileName.endsWith(".dot",Qt::CaseInsensitive ) ) {
            m_fileFormat=FILE_GRAPHVIZ;
        }
        else if (m_fileName.endsWith(".gml",Qt::CaseInsensitive ) ) {
            m_fileFormat=FILE_GML;
        }
        else if (m_fileName.endsWith(".list",Qt::CaseInsensitive ) ||
                 m_fileName.endsWith(".lst",Qt::CaseInsensitive )  ) {
            m_fileFormat=FILE_EDGELIST_SIMPLE;
        }
        else if (m_fileName.endsWith(".wlist",Qt::CaseInsensitive ) ||
                 m_fileName.endsWith(".wlst",Qt::CaseInsensitive )  ) {
            m_fileFormat=FILE_EDGELIST_WEIGHTED;
        }
        else if (m_fileName.endsWith(".2sm",Qt::CaseInsensitive ) ||
                 m_fileName.endsWith(".aff",Qt::CaseInsensitive )  ) {
            m_fileFormat=FILE_TWOMODE;
        }
        else
            m_fileFormat=FILE_UNRECOGNIZED;
    }


    qDebug()<<"MW::slotNetworkFileChoose() - Calling slotNetworkFilePreview"
           << "with m_fileName" << m_fileName
           << "and m_fileFormat " << m_fileFormat;

    slotNetworkFilePreview(m_fileName, m_fileFormat );


}




void MainWindow::slotNetworkFileDialogRejected() {
    qDebug() << "MW::slotNetworkFileDialogRejected() - if a file was previously opened, get back to it.";
    statusMessage( tr("Opening aborted"));
}


/**
 * @brief Called when user selects a file filter (i.e. GraphML) in the fileDialog
 * @param filter
 */
void MainWindow::slotNetworkFileDialogFilterSelected(const QString &filter) {
    qDebug() << "MW::slotNetworkFileDialogFilterSelected() - filter" << filter;
    if (filter.startsWith("GraphML",Qt::CaseInsensitive ) ) {
        fileType=FILE_GRAPHML;
        qDebug() << "MW::slotNetworkFileDialogFilterSelected() - fileType FILE_GRAPHML";
    }
    else if (filter.contains("PAJEK",Qt::CaseInsensitive ) ) {
        fileType=FILE_PAJEK;
        qDebug() << "MW::slotNetworkFileDialogFilterSelected() - fileType FILE_PAJEK";
    }
    else if (filter.contains("DL",Qt::CaseInsensitive ) ||
             filter.contains("UCINET",Qt::CaseInsensitive ) ) {
        fileType=FILE_UCINET;
        qDebug() << "MW::slotNetworkFileDialogFilterSelected() - fileType FILE_UCINET";
    }
    else if (filter.contains("Adjancency",Qt::CaseInsensitive ) ) {
        fileType=FILE_ADJACENCY;
        qDebug() << "MW::slotNetworkFileDialogFilterSelected() - fileType FILE_ADJACENCY";
    }
    else if (filter.contains("GraphViz",Qt::CaseInsensitive ) ) {
        fileType=FILE_GRAPHVIZ;
        qDebug() << "MW::slotNetworkFileDialogFilterSelected() - fileType FILE_GRAPHVIZ";
    }
    else if (filter.contains("GML",Qt::CaseInsensitive ) ) {
        fileType=FILE_GML;
        qDebug() << "MW::slotNetworkFileDialogFilterSelected() - fileType FILE_GML";
    }
    else if (filter.contains("Simple Edge List",Qt::CaseInsensitive ) ) {
        fileType=FILE_EDGELIST_SIMPLE;
        qDebug() << "MW::slotNetworkFileDialogFilterSelected() - fileType FILE_EDGELIST_SIMPLE";
    }
    else if (filter.contains("Weighted Edge List",Qt::CaseInsensitive ) ) {
        fileType=FILE_EDGELIST_WEIGHTED;
        qDebug() << "MW::slotNetworkFileDialogFilterSelected() - fileType FILE_EDGELIST_WEIGHTED";
    }
    else if (filter.contains("Two-Mode",Qt::CaseInsensitive )  ) {
        fileType=FILE_TWOMODE;
        qDebug() << "MW::slotNetworkFileDialogFilterSelected() - fileType FILE_TWOMODE";
    }
    else {
        fileType=FILE_UNRECOGNIZED;
        qDebug() << "MW::slotNetworkFileDialogFilterSelected() - fileType FILE_UNRECOGNIZED";
    }


}


/**
 * @brief Called when user selects a file in the fileDialog
 * Calls slotNetworkFileChoose() again.
 * @param fileName
 *
 */
void MainWindow::slotNetworkFileDialogFileSelected(const QString &fileName) {
    qDebug() << "MW::slotNetworkFileDialogFileSelected() - filename " << fileName
             << "calling slotNetworkFileChoose() with fileType" << fileType;
    slotNetworkFileChoose( fileName,
                           fileType,
                           (  (fileType==FILE_UNRECOGNIZED) ? true : false )
                           );
}


/**
 * @brief Saves the network to a file.
 * First check if a fileName is currently used
 * If not, calls slotNetworkSaveAs (which prompts for a fileName before returning here)
 * If a fileName is currently set, it checks if fileFormat is supported for export
 * If not supported, and the file is new, just tries to save in GraphML
 * For other exporing options the user is informed to use the export menu.
 */
void MainWindow::slotNetworkSave(const int &fileFormat) {
    statusMessage( tr("Saving file..."));

    if (activeNodes() == 0) {
        statusMessage(  QString(tr("Nothing to save. There are no vertices.")) );
    }
    if (activeGraph.graphSaved()) {
        statusMessage(  QString(tr("Graph already saved.")) );
    }
    if ( fileName.isEmpty() ) {
        slotNetworkSaveAs();
        return;
    }
    QFileInfo fileInfo (fileName);
    fileNameNoPath = fileInfo.fileName();

    if ( activeGraph.graphFileFormatExportSupported( fileFormat ) )
    {
        activeGraph.graphSave(fileName, fileFormat ) ;
    }
    else if (activeGraph.graphFileFormat()==FILE_GRAPHML ||
             ( activeGraph.graphSaved() && !activeGraph.graphLoaded() )
             )
    {	//new file or GraphML
        activeGraph.graphSave(fileName, FILE_GRAPHML);
    }

    else if ( activeGraph.graphFileFormatExportSupported(
                  activeGraph.graphFileFormat()
                  ) )
    {
        activeGraph.graphSave(fileName, activeGraph.graphFileFormat() ) ;
    }
    else
    {
        switch(
               slotHelpMessageToUser (USER_MSG_QUESTION,
                                      tr("Save to GraphML?"),
                                      tr("Default File Format: GraphML "),
                                      tr("This network will be saved in GraphML format "
                                         "which is the default file format of SocNetV. \n\n"
                                         "Is this OK? \n\n"
                                         "If not, press Cancel, then go to Network > Export menu "
                                         "to see other supported formats to export your data to.")
                                      )
               )
        {
        case QMessageBox::Yes:
             activeGraph.graphSave(fileName, FILE_GRAPHML);
            break;
        case QMessageBox::Cancel:
        case QMessageBox::No:
            statusMessage( tr("Save aborted...") );
            break;
        }
    }

}




/**
 * @brief Saves the network in a new file
 */
void MainWindow::slotNetworkSaveAs() {
    qDebug() << "MW::slotNetworkSaveAs()";
    statusMessage( tr("Enter or select a filename to save the network..."));

    QString fn =  QFileDialog::getSaveFileName(
                this,
                tr("Save Network to GraphML File Named..."),
                getLastPath(), tr("GraphML (*.graphml *.xml);;All (*)") );
    if (!fn.isEmpty())  {
        if  ( QFileInfo(fn).suffix().isEmpty() ){
            slotHelpMessageToUser (
                        USER_MSG_INFO,
                        tr("Appending .graphml"),
                        tr("Missing Extension. \n"
                           "Appending a standard .graphml to the given filename.")
                        );
            fn.append(".graphml");
        }
        /** \todo  Change the suffix automatically to graphML even if the user
         * has selected other?
         */
        if ( !QFileInfo(fn).suffix().contains("graphML") ||
             !QFileInfo(fn).suffix().contains("xml") ) {
            //fn = QFileInfo(fn).absoluteDir() + QFileInfo(fn).baseName()
        }
        fileName=fn;
        QFileInfo fileInfo (fileName);
        fileNameNoPath = fileInfo.fileName();
        setLastPath(fileName); // store this path
        slotNetworkSave(FILE_GRAPHML);
    }
    else  {
        statusMessage( tr("Saving aborted"));
        return;
    }
}



/**
 * @brief Called from Graph when we save file.
 * Updates Save icon and window title.
 * @param saved_ok
 *
 */
void MainWindow::slotNetworkSaved(const int &status)
{
    if (status <= 0) {
        statusMessage( tr("Error! Could not save this file: %1").arg (fileNameNoPath));
    }
    else {
        networkSave->setIcon(QIcon(":/images/saved.png"));
        networkSave->setEnabled(false);
        setWindowTitle( fileNameNoPath );
        statusMessage( tr("Network saved under filename: %1").arg (fileNameNoPath));
    }
}



/**
 * @brief Closes the network. Saves it if necessary. Used by createNew.
 */
void MainWindow::slotNetworkClose() {
    qDebug()<<"slotNetworkClose()";
    statusMessage( tr("Closing network file..."));
    if (!activeGraph.graphSaved()) {
        switch (
                slotHelpMessageToUser (
                            USER_MSG_QUESTION,
                            tr("Closing Network..."),
                            tr("Network has not been saved. \n"
                               "Do you want to save before closing it?")
                            )
                )
        {
        case QMessageBox::Yes: slotNetworkSave(); break;
        case QMessageBox::No: break;
        case QMessageBox::Cancel: return; break;
        }
    }
    statusMessage( tr("Erasing old network data...."));
    initApp();
    statusMessage( tr("Ready."));
}



/**
 * @brief Sends the active network to the printer
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
 * @brief Imports a network from a GraphML formatted file
 */
void MainWindow::slotNetworkImportGraphML(){
    bool m_checkSelectFileType = false;
    slotNetworkFileChoose( QString::null, FILE_GRAPHML, m_checkSelectFileType);
}



/**
 * @brief Imports a network from a GML formatted file
 */
void MainWindow::slotNetworkImportGML(){
    bool m_checkSelectFileType = false;
    slotNetworkFileChoose( QString::null, FILE_GML, m_checkSelectFileType);
}

/**
 * @brief Imports a network from a Pajek-like formatted file
 */
void MainWindow::slotNetworkImportPajek(){
    bool m_checkSelectFileType = false;
    slotNetworkFileChoose( QString::null, FILE_PAJEK, m_checkSelectFileType);
}




/**
 * @brief Imports a network from a Adjacency matrix formatted file
 */
void MainWindow::slotNetworkImportSM(){
    bool m_checkSelectFileType = false;
    slotNetworkFileChoose( QString::null, FILE_ADJACENCY, m_checkSelectFileType);
}




/**
 * @brief Imports a network from a Dot (GraphViz) formatted file
 */
void MainWindow::slotNetworkImportDot(){
    bool m_checkSelectFileType = false;
    slotNetworkFileChoose( QString::null ,FILE_GRAPHVIZ, m_checkSelectFileType);
}







/**
 * @brief Imports a network from a UCINET formatted file
 */
void MainWindow::slotNetworkImportDL(){
    bool m_checkSelectFileType = false;
    slotNetworkFileChoose( QString::null, FILE_UCINET, m_checkSelectFileType);
}



/**
 * @brief Imports a network from a simple List or weighted List formatted file
 */
void MainWindow::slotNetworkImportEdgeList(){
    bool m_checkSelectFileType = false;

    switch(
           slotHelpMessageToUser(USER_MSG_QUESTION_CUSTOM,
                                 tr("Select type of edge list format..."),
                                 tr("Select type of edge list format"),
                                 tr("SocNetV can parse two kinds of edgelist formats: \n\n"
                                    "A. Edge lists with edge weights, "
                                    "where each line has exactly 3 columns: "
                                    "source  target  weight, i.e.:\n"
                                    "1 2 1 \n"
                                    "2 3 1 \n"
                                    "3 4 2 \n"
                                    "4 5 1 \n\n"
                                    "B. Simple edge lists without weights, where each line "
                                    "has two or more columns in the form: source, target1, target2, ... , i.e.:\n"
                                    "1 2 3 4 5 6\n"
                                    "2 3 4 \n"
                                    "3 5 8 7\n\n"
                                 "Please select the appropriate type of edge list format of "
                                 "the file you want to load:"),
                                 QMessageBox::NoButton, QMessageBox::NoButton,
                                 tr("Weighted"), tr("Simple non-weighted")

                                 )
           )
    {
    case 1:
        qDebug() << "***  MW::slotNetworkImportEdgeList - Weighted list selected! " ;
        slotNetworkFileChoose( QString::null, FILE_EDGELIST_WEIGHTED, m_checkSelectFileType);
        break;
    case 2:
        qDebug() << "***  MW: slotNetworkImportEdgeList - Simple list selected! " ;
        slotNetworkFileChoose( QString::null, FILE_EDGELIST_SIMPLE, m_checkSelectFileType);
        break;
    }
}



/**
 * @brief Imports a network from a two mode sociomatrix formatted file
 */
void MainWindow::slotNetworkImportTwoModeSM(){
    bool m_checkSelectFileType = false;
    slotNetworkFileChoose( QString::null, FILE_TWOMODE, m_checkSelectFileType);
}



/**
 * @brief Setup a list of all text codecs supported by OS
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
 * @brief Called from slotNetworkFileChoose()
 * Opens a window to preview the selected file where the user
 * can select an appropriate text codec
 * @param m_fileName
 * @param m_fileFormat
 * @return
 */
bool MainWindow::slotNetworkFilePreview(const QString &m_fileName,
                                    const int &m_fileFormat ){
    qDebug() << "MW::slotNetworkFilePreview() - file: "<< m_fileName;

    if (!m_fileName.isEmpty()) {
        QFile file(m_fileName);
        if (!file.open(QFile::ReadOnly)) {
            slotHelpMessageToUserError(
                        tr("Cannot read file %1:\n%2")
                        .arg(m_fileName)
                        .arg(file.errorString())
                                       );
            return false;
        }
        qDebug() << "MW::slotNetworkFilePreview() - reading file... " ;
        QByteArray data = file.readAll();

        m_dialogPreviewFile->setEncodedData(data,m_fileName, m_fileFormat);
        m_dialogPreviewFile->exec();
    }
    return true;
}




/**
 * @brief Called on click on any file entry in "Recent Files" menu
 * Calls slotNetworkFileChoose() which checks file type and calls slotNetworkFilePreview
 */
void MainWindow::slotNetworkFileLoadRecent() {
    QAction *action = qobject_cast<QAction *>(sender());
    if (action) {
        slotNetworkFileChoose(action->data().toString() );
    }
}




/**
 * @brief Main network file loader method
 * Called from m_dialogPreviewFile and slotNetworkDataSetRecreate
 * Calls initApp to init to default values.
 * Then calls activeGraph::graphLoad to actually load the network...
 * @param m_fileName
 * @param m_codecName
 * @param m_fileFormat
 */
void MainWindow::slotNetworkFileLoad(const QString m_fileName,
                                 const QString m_codecName,
                                 const int m_fileFormat )
{
    qDebug() << "MW::slotNetworkFileLoad() : "<< m_fileName
                    << " m_codecName " << m_codecName
                    << " m_fileFormat " << m_fileFormat;
    initApp();

    userSelectedCodecName = m_codecName; //var for future use in a Settings dialog
    QString delimiter=QString::null;
    int two_sm_mode = 0;

    if ( m_fileFormat == FILE_TWOMODE ) {
        switch(
               slotHelpMessageToUser (
                   USER_MSG_QUESTION_CUSTOM,
                   tr("Two-mode sociomatrix. Select mode..."),
                   tr("Two-mode sociomatrix"),
                   tr("If this file is in two-mode sociomatrix format, "
                      "please specify which mode to open \n\n"
                      "1st mode: rows are nodes \n"
                      "2nd mode: columns are nodes"),
                   QMessageBox::NoButton,
                   QMessageBox::Ok,
                   tr("1st Mode"),tr("2nd mode")

                           )
               ) {
        case 1:
            two_sm_mode = 1;
            break;
        case 2:
            two_sm_mode = 2;
            break;
        }
    }

    if ( m_fileFormat == FILE_EDGELIST_SIMPLE ||
            m_fileFormat == FILE_EDGELIST_WEIGHTED ) {
        bool ok;
        QString delimiter =
                QInputDialog::getText(this, tr("Enter column delimiter"),
                                      tr("SocNetV supports edge list formatted files"
                                         "with arbitrary column delimiters. \n"
                                         "The default delimiter is one or more spaces.\n"
                                         "If the column delimiter in this file is "
                                         "other than simple space or TAB, \n"
                                         "please enter it below.\n"
                                         "For instance, if the delimiter is a "
                                         "comma or pipe enter \",\" or \"|\" respectively.\n"
                                         "Leave empty to use space or TAB as delimiter."),
                                      QLineEdit::Normal,
                                      QString(""), &ok);
        if (!ok || delimiter.isEmpty() || delimiter.isNull() ) {
            delimiter=" ";
        }
        qDebug()<<"MW::slotNetworkFileLoad() - delimiter" << delimiter;
    }
    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
    qDebug() << "MW::slotNetworkFileLoad() : calling activeGraph.graphLoad() ";

    activeGraph.graphLoad (
                m_fileName,
                m_codecName,
                ((appSettings["initNodeLabelsVisibility"] == "true" ) ? true: false),
            m_fileFormat,
            two_sm_mode,
            delimiter
            );

    QApplication::restoreOverrideCursor();

}




/**
 * @brief Called from Parser/Graph when a network file is loaded.
 * It informs the MW about the type of the network so that it can display the appropiate message.
 * @param type
 * @param netName
 * @param aNodes
 * @param totalEdges
 */
void MainWindow::slotNetworkFileLoaded (const int &type,
                                        const QString &fName,
                                        const QString &netName,
                                        const int &totalNodes,
                                        const int &totalEdges,
                                        const QString &message)
{
    qDebug()<< "MW::slotNetworkFileLoaded() - type " << type;

    if (type > 0) {
        fileName=fName;
        previous_fileName=fileName;
        QFileInfo fileInfo (fileName);
        fileNameNoPath = fileInfo.fileName();

        Q_ASSERT_X( !fileNameNoPath.isEmpty(),  "not empty filename ", "empty filename " );

        setWindowTitle("SocNetV "+ VERSION +" - "+fileNameNoPath);
        setLastPath(fileName); // store this path and file
    }
    else {

        qDebug()<< "MW::slotNetworkFileLoaded() - UNRECOGNIZED FILE. "
                   "Message from Parser: "
                << message
                   << "Calling initApp()";

        statusMessage( tr("Error loading requested file. Aborted."));

        slotHelpMessageToUser(USER_MSG_CRITICAL,
                              tr("Error loading network file"),
                              tr("Error loading network file"),
                              tr("Sorry, the selected file is not in a supported format or encoding, "
                                 "or contains formatting errors. \n\n"
                                 "The error message was: \n\n"
                                 "%1"
                                 "\n\n"
                              "What now? Review the message above to see if it helps you to fix the data file. "
                                 "Try a different codec in the preview window "
                                 "or if the file is of a legacy format (i.e. Pajek, UCINET, GraphViz, etc), "
                              "please use the options in the Import sub menu. \n").arg(message)
                              );

        initApp();

        return;
    }

    switch( type ) 	{
    case 0:
        break;
    case 1:
        statusMessage( tr("GraphML formatted network, named %1, loaded with %2 Nodes and %3 total Edges.").arg( netName ).arg( totalNodes ).arg(totalEdges ) );
        break;

    case 2:
        statusMessage( QString(tr("Pajek formatted network, named %1, loaded with %2 Nodes and %3 total Edges.")).arg( netName ).arg( totalNodes ).arg(totalEdges ));
        break;

    case 3:
        statusMessage( QString(tr("Adjacency formatted network, named %1, loaded with %2 Nodes and %3 total Edges.")).arg( netName ).arg( totalNodes ).arg(totalEdges ) );
        break;

    case 4:
        statusMessage( QString(tr("GraphViz (Dot) formatted network, named %1, loaded with %2 Nodes and %3 total Edges.")).arg( netName ).arg( totalNodes ).arg(totalEdges ) );
        break;

    case 5:
        statusMessage( QString(tr("UCINET formatted network, named %1, loaded with %2 Nodes and %3 total Edges.")).arg( netName ).arg( totalNodes ).arg(totalEdges ) );
        break;
    case 6:
        statusMessage( QString(tr("GML formatted network, named %1, loaded with %2 Nodes and %3 total Edges.")).arg( netName ).arg( totalNodes ).arg(totalEdges ) );
        break;
    case 7:
        statusMessage( QString(tr("Weighted list formatted network, named %1, loaded with %2 Nodes and %3 total Edges.")).arg( netName ).arg( totalNodes ).arg(totalEdges ) );
        break;
    case 8:
        statusMessage( QString(tr("Simple list formatted network, named %1, loaded with %2 Nodes and %3 total Edges.")).arg( netName ).arg( totalNodes ).arg(totalEdges ) );
        break;
    case 9:
        statusMessage( QString(tr("Two-mode affiliation network, named %1, loaded with %2 Nodes and %3 total Edges.")).arg( netName ).arg( totalNodes ).arg(totalEdges ) );
        break;

    default: // just for sanity
         QMessageBox::critical(this, "Error","Unrecognized format. \nPlease specify"
                              " which is the file-format using Import Menu.","OK",0);
        break;
    }
    networkSave->setIcon(QIcon(":/images/saved.png"));
    networkSave->setEnabled(false);
}



/**
 * @brief Called from Graph::relationsClear() to clear the relations combo.
 */
void MainWindow::slotEditRelationsClear(){
    qDebug() << "MW::slotEditRelationsClear() - clearing combo";
    editRelationChangeCombo->clear();
}

/**
 * @brief Called from MW when user clicks New Relation btn
 * or when the user creates the first edge visually.
 * Called from activeGraph::relationAdd(QString)
 * via signal Graph::signalRelationChangedToMW() when the parser or a
 * Graph method demands a new relation to be added in the Combobox.
 */
void MainWindow::slotEditRelationAdd(QString newRelationName, const bool &changeRelation){
    int comboItemsBefore = editRelationChangeCombo->count();
    int relationsCounter=activeGraph.relations();

    qDebug() << "MW::slotEditRelationAdd() - adding relation:"
             << newRelationName
             <<"to relations combo. Before this, combo items:"
            << comboItemsBefore
            << "and currentIndex:"
            <<editRelationChangeCombo->currentIndex()
           << "relationsCounter:"
           <<relationsCounter;

    if (!newRelationName.isNull()) {

        editRelationChangeCombo->addItem(newRelationName);

        if (changeRelation) {
            if ( comboItemsBefore == 0 ) { // only at startup
                slotEditRelationChange(0);
            }
            else {
                slotEditRelationChange();
            }

        }
        qDebug() << "MW::slotEditRelationAdd() - added relation:"
                 << newRelationName
                <<"now combo items:"
               << editRelationChangeCombo->count()
                << "now currentIndex:"
                <<editRelationChangeCombo->currentIndex()
               << "relationsCounter"
               <<relationsCounter;
        return;
    }

    bool ok;

    if (relationsCounter==1 && activeNodes()==0 ) {
        newRelationName = QInputDialog::getText(
                    this,
                    tr("Add new relation"),
                    tr("Enter a name for a new relation between the actors.\n"
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
        // user pressed OK, name entered
        emit signalRelationAddAndChange(newRelationName);
    }
    else if ( newRelationName.isEmpty() && ok ){
        // user pressed OK, no name entered
        QMessageBox::critical(this, tr("Error"),
                              tr("You did not type a name for this new relation"),
                              QMessageBox::Ok, 0);
        slotEditRelationAdd();
    }
    else {
        //user pressed Cancel
        statusMessage( QString(tr("New relation cancelled.")) );
        return;
    }
    statusMessage( QString(tr("New relation named %1, added."))
                   .arg( newRelationName ) );
}




/**
 * @brief if relIndex==RAND_MAX changes combo box index to last relation index
 * else it changes the combo box index to relIndex
 * Called from Graph::relationAddAndChangeTo
 * via signal Graph::signalRelationChangedToMW()
 * @param relIndex
 */
void MainWindow::slotEditRelationChange(const int relIndex) {
    if ( relIndex == RAND_MAX){
        qDebug() << "MW::slotEditRelationChange(int) - RANDMAX. Change to last relation";
        editRelationChangeCombo->setCurrentIndex(
                     ( editRelationChangeCombo->count()-1 )
                    );
    }
    else {
        qDebug() << "MW::slotEditRelationChange(int) - to index" << relIndex;
        editRelationChangeCombo->setCurrentIndex(relIndex);
    }

}




/**
 * @brief Renames a relation
 * @param newName
 */
void MainWindow::slotEditRelationRename(QString newName) {
    qDebug()<<"MW::slotEditRelationRename() -" << newName;
    bool ok=false;
    if (newName.isNull() || newName.isEmpty()) {
        qDebug()<<"MW::slotEditRelationRename() - prompt to enter new name";
        newName = QInputDialog::getText(
                    this,
                    tr("Rename current relation"),
                    tr("Enter a new name for this relation."),
                              QLineEdit::Normal, QString::null, &ok );
        if ( newName.isEmpty() || !ok ){
            slotHelpMessageToUser(USER_MSG_CRITICAL,
                                  tr("Not a valid name."),
                                  tr("Error"),
                                  tr("You did not enter a valid name for this relation.")
                                  );
            return;
        }
        else {
            activeGraph.relationCurrentRename(newName, true);
        }
    }
    else {
        qDebug()<<"MW::slotEditRelationRename() - current text " << editRelationChangeCombo->currentText();
        qDebug()<<"MW::slotEditRelationRename() - updating combo name to" << newName;
        editRelationChangeCombo->setCurrentText(newName);
    }

}







/**
 * @brief Exports the network to a PNG image - Mediocre Quality but smaller file
 * @return
 *
 */
bool MainWindow::slotNetworkExportPNG(){
    qDebug()<< "MW::slotNetworkExportPNG";
    if ( !activeNodes() )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
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
 * @brief Exports the network to a BMP image - Better Quality but larger file
 * @return
 */
bool MainWindow::slotNetworkExportBMP(){
    qDebug(	"slotNetworkExportBMP()");
    if ( !activeNodes() )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
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
 * @brief Exports the network to a PDF Document - Best Quality
 * @return
 *
 */
bool MainWindow::slotNetworkExportPDF(){
    qDebug()<< "MW::slotNetworkExportPDF()";
    if ( !activeNodes() )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
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

        // dont set to HighResolution - it breaks pdf export
        QPrinter printer(QPrinter::ScreenResolution);
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
 * @brief Exports the network to a Pajek-formatted file
 * Calls the relevant Graph method.
 */
void MainWindow::slotNetworkExportPajek()
{
    qDebug () << "MW::slotNetworkExportPajek";

    if ( !activeNodes() )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    statusMessage( tr("Exporting active network under new filename..."));
    QString fn =  QFileDialog::getSaveFileName(
                this,
                tr("Export Network to File Named..."),
                getLastPath(), tr("Pajek (*.paj *.net *.pajek);;All (*)") );
    if (!fn.isEmpty())  {
        if  ( QFileInfo(fn).suffix().isEmpty() ){
            QMessageBox::information(this, "Missing Extension ",
                                     tr("File extension was missing! \n"
                                        "Appending a standard .paj to the given filename."), "OK",0);
            fn.append(".paj");
        }
        fileName=fn;
        setLastPath(fileName);
        QFileInfo fileInfo (fileName);
        fileNameNoPath = fileInfo.fileName();
    }
    else  {
        statusMessage( tr("Saving aborted"));
        return;
    }

    activeGraph.graphSave(fileName, FILE_PAJEK);
}



/**
 * @brief Exports the network to a adjacency matrix-formatted file
 * Calls the relevant Graph method.
 */
void MainWindow::slotNetworkExportSM(){
    qDebug("MW: slotNetworkExportSM()");
    if ( !activeNodes() )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    statusMessage( tr("Exporting active network under new filename..."));
    QString fn =  QFileDialog::getSaveFileName(
                this,
                tr("Export Network to File Named..."),
                getLastPath(), tr("Adjacency (*.adj *.sm *.txt *.csv *.net);;All (*)") );
    if (!fn.isEmpty())  {
        if  ( QFileInfo(fn).suffix().isEmpty() ){
            QMessageBox::information(this, "Missing Extension ",
                                     tr("File extension was missing! \n"
                                        "Appending a standard .adj to the given filename."), "OK",0);
            fn.append(".adj");
        }
        fileName=fn;
        setLastPath(fileName);
        QFileInfo fileInfo (fileName);
        fileNameNoPath = fileInfo.fileName();
    }
    else  {
        statusMessage( tr("Saving aborted"));
        return;
    }


    bool saveEdgeWeights=false;
    if (activeGraph.graphWeighted() )  {
        switch (
                slotHelpMessageToUser(USER_MSG_QUESTION,
                                  tr("Weighted graph. Social network with valued/weighted edges"),
                                  tr("Social network with valued/weighted edges"),
                                  tr("This social network includes valued/weighted edges "
                                     "(the depicted graph is weighted). "
                                     "Do you want to save the edge weights in the adjacency file?\n"
                                     "Select Yes if you want to save edge values "
                                     "in the resulting file. \n"
                                     "Select No, if you don't want edge values "
                                     "to be saved. In the later case, all non-zero values will be truncated to 1.")
                                  )

                )
        {
        case QMessageBox::Yes:
            saveEdgeWeights = true;
            break;
        case QMessageBox::No:
            saveEdgeWeights = false;
            break;
        case QMessageBox::Cancel:
            statusMessage( tr("Save aborted...") );
            return;
            break;
        }

    }

    activeGraph.graphSave(fileName, FILE_ADJACENCY,  saveEdgeWeights ) ;

}





/**
 * @brief TODO Exports the network to a DL-formatted file
 * @return
 */
bool MainWindow::slotNetworkExportDL(){
    if ( !activeNodes() )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
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
    TODO: Exports the network to a GW-formatted file
*/ 
bool MainWindow::slotNetworkExportGW(){
    if ( !activeNodes() )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
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
    TODO: Exports the network to a list-formatted file
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
 * @brief Displays the file of the loaded network.
   Network _must_ be unchanged since last save/load.
   Otherwise it will ask the user to first save the network, then view its file.
 */
void MainWindow::slotNetworkFileView(){
    qDebug() << "slotNetworkFileView() : " << fileName.toLatin1();
    if ( activeGraph.graphLoaded() && activeGraph.graphSaved()  ) {
        //network unmodified, read loaded file again.
        QFile f( fileName );
        if ( !f.open( QIODevice::ReadOnly ) ) {
            qDebug ("Error in open!");
            return;
        }
        TextEditor *ed = new TextEditor(fileName,this,false);
        QFileInfo fileInfo (fileName);
        fileNameNoPath = fileInfo.fileName();
        ed->setWindowTitle( fileNameNoPath );
        ed->show();
        m_textEditors << ed;
        statusMessage(  tr("Displaying network data file %1" ).arg(fileNameNoPath));
    }

    else if (!activeGraph.graphSaved() ) {

        if ( !activeGraph.graphLoaded() ) {
            // new network, not saved yet
            int response = slotHelpMessageToUser(USER_MSG_QUESTION,
                                              tr("New network not saved yet. You might want to save it first."),
                                              tr("This new network you created has not been saved yet."),
                                              tr("Do you want to open a file dialog to save your work "
                                                 "(then I will display the file)?"),
                                                   QMessageBox::Yes|QMessageBox::No,QMessageBox::Yes
                                              );
            if (  response == QMessageBox::Yes ) {
                slotNetworkSaveAs();
            }
            else { return; }
        }
        else {
            // loaded network, but modified
            int response = slotHelpMessageToUser(USER_MSG_QUESTION,
                                              tr("Current network has been modified. Save to the original file?"),
                                              tr("Current social network has been modified since last save."),
                                              tr("Do you want to save it to the original file?"),
                                              QMessageBox::Yes|QMessageBox::No,QMessageBox::Yes
                                              );
            if ( response ==  QMessageBox::Yes ){
                slotNetworkSave();
            }else if (response ==QMessageBox::No ) {
                slotNetworkSaveAs();
            }
            else { // user pressed Cancel
                return;
            }

        }
        slotNetworkFileView();
    }
    else	{
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
    }
}




/**
 * @brief Opens the embedded text editor
 */
void MainWindow::slotNetworkTextEditor(){
    qDebug() << "slotNetworkTextEditor() : ";

    TextEditor *ed = new TextEditor("", this,false);
    ed->setWindowTitle(tr("New Network File"));
    ed->show();
    m_textEditors << ed;
    statusMessage(  tr("Enter your network data here" ) );
}





/**
 * @brief Displays the adjacency matrix of the network.
 *  It uses a different method for writing the matrix to a file.
 *  While slotNetworkExportSM uses << operator of Matrix class
 *  (via adjacencyMatrix of Graph class), this is using directly the
 *  writeMatrixAdjacency method of Graph class
 */
void MainWindow::slotNetworkViewSociomatrix(){
    if ( !activeNodes() ) {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-matrix-adjacency-"+dateTime+".html";

    qDebug () << "MW::slotNetworkViewSociomatrix() - dataDir"
              << appSettings["dataDir"]
              << "fn" <<fn;

    statusMessage ( tr ("Creating and writing adjacency matrix") );

    activeGraph.writeMatrixAdjacency(fn) ;
    //AVOID THIS, no preserving of node numbers when nodes are deleted.
    // activeGraph.writeMatrix(fn,MATRIX_ADJACENCY) ;

    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {

        qDebug () << "MW::slotNetworkViewSociomatrix() - "
                     "calling QDesktopServices::openUrl for"
                  << QUrl::fromLocalFile(fn) ;

        QDesktopServices::openUrl(  QUrl::fromLocalFile(fn)  );
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }
    statusMessage(tr("Adjacency matrix saved as ") + QDir::toNativeSeparators(fn));
}



/**
 * @brief Displays a text-only plot of the network adjacency matrix
 */
void MainWindow::slotNetworkViewSociomatrixPlotText(){
    if ( !activeNodes() ) {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    int N=activeNodes();

    statusMessage(tr("Creating plot of adjacency matrix of %1 nodes.").arg(N ));

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-matrix-adjacency-plot-"+dateTime+".html";

    bool simpler = false;
    if ( N > 999 ) {
        float MB = (N * N * 10)/(1024*1024);
        switch ( slotHelpMessageToUser(
                     USER_MSG_QUESTION,tr("Very large network to plot!"),
                     tr("Warning: Really large network"),
                     tr("To plot a %1 x %1 matrix arranged in HTML table, "
                        "I will need time to write a very large .html file , circa %2 MB in size. "
                        "Instead, I can create a simpler / smaller HTML file without table. "
                        "Press Yes to continue with simpler version, "
                        "Press No to create large file with HTML table.").arg(N).arg( MB ) ) ) {
        case QMessageBox::Yes:
            simpler = true;
            break;
        case QMessageBox::No:
            simpler = false;
            break;
        default:
            return;
            break;
        }
    }


    activeGraph.writeMatrixAdjacencyPlot(fn, simpler);

    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage(tr("Visual form of adjacency matrix saved as ") + QDir::toNativeSeparators(fn));
}



/**
 * @brief Calls the m_datasetSelectionDialog to display the dataset selection dialog
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

    //initApp();

    qDebug()<< "MW::slotNetworkDataSetRecreate() datadir+fileName: "
            << appSettings["dataDir"]+m_fileName;
    activeGraph.writeDataSetToFile(appSettings["dataDir"], m_fileName);

    if (m_fileName.endsWith(".graphml")) {
        m_fileFormat=FILE_GRAPHML;
    }
    else if (m_fileName.endsWith(".pajek") || m_fileName.endsWith(".paj") ||
             m_fileName.endsWith(".net")) {
        m_fileFormat=FILE_PAJEK;
    }
    else if (m_fileName.endsWith(".sm") || m_fileName.endsWith(".adj")) {
        m_fileFormat=FILE_ADJACENCY;
    }
    else if (m_fileName.endsWith(".dot")) {
        m_fileFormat=FILE_GRAPHVIZ;
    }
    else if (m_fileName.endsWith(".dl")) {
        m_fileFormat=FILE_UCINET;
    }
    else if (m_fileName.endsWith(".gml")) {
        m_fileFormat=FILE_GML;
    }
    else if (m_fileName.endsWith(".wlst")) {
        m_fileFormat=FILE_EDGELIST_WEIGHTED;
    }
    else if (m_fileName.endsWith(".lst")) {
        m_fileFormat=FILE_EDGELIST_SIMPLE;
    }
    else if (m_fileName.endsWith(".2sm")) {
        m_fileFormat=FILE_TWOMODE;
    }

    slotNetworkFileLoad(appSettings["dataDir"]+m_fileName, "UTF-8", m_fileFormat);

//    if ( slotNetworkFileLoad(appSettings["dataDir"]+m_fileName, "UTF-8", m_fileFormat) ) {
//        qDebug() << "slotNetworkDataSetRecreate() loaded file " << m_fileName;
//        fileName=m_fileName;
//        previous_fileName=fileName;
//        setWindowTitle("SocNetV "+ VERSION +" - "+fileName);
//        QString message=tr("Dataset loaded. Dataset file saved as ") + fileName;
//        statusMessage( message );
//    }
//    else {
//        statusMessage( "Could not read new network data file. Aborting.");
//    }
}


/**
 * @brief MainWindow::slotNetworkRandomErdosRenyiDialog
 * Shows the Erdos-Renyi network creation dialog
 */
void MainWindow::slotNetworkRandomErdosRenyiDialog(){

    statusMessage( tr("Generate a random Erdos-Renyi network. "));

    m_randErdosRenyiDialog = new DialogRandErdosRenyi(
                this, appSettings["randomErdosEdgeProbability"].toFloat(0));

    connect( m_randErdosRenyiDialog, &DialogRandErdosRenyi::userChoices,
             this, &MainWindow::slotNetworkRandomErdosRenyi );

    m_randErdosRenyiDialog->exec();

}




/**
 * @brief MainWindow::slotNetworkRandomErdosRenyi
 * @param newNodes
 * @param model
 * @param edges
 * @param eprob
 * @param mode
 * @param diag
 * Calls activeGraph.slotNetworkRandomErdosRenyi () to create a symmetric network
 * Edge existance is controlled by a user specified possibility.
 */
void MainWindow::slotNetworkRandomErdosRenyi( const int newNodes,
                                       const QString model,
                                       const int edges,
                                       const float eprob,
                                       const QString mode,
                                       const bool diag)
{
    qDebug() << "MW::slotNetworkRandomErdosRenyi()";

    initApp();

    statusMessage( tr("Creating Erdos-Renyi Random Network. Please wait... ")  );

    appSettings["randomErdosEdgeProbability"] = QString::number(eprob);


    activeGraph.randomNetErdosCreate ( newNodes,
                                       model,
                                       edges,
                                       eprob,
                                       mode,
                                       diag);


    setWindowTitle("Untitled Erdos-Renyi random network");

    double threshold = log(newNodes)/newNodes;

    //float clucof=activeGraph.clusteringCoefficient();

    if ( (eprob ) > threshold )
        QMessageBox::information(
                    this,
                    "New Erdos-Renyi Random Network",
                    tr("Random network created. \n")+
                    //tr("\nAverage path length: ") + QString::number(avGraphDistance)+
                    //tr("\nClustering coefficient: ")+QString::number(clucof)+
                    tr("\n\nOn the average, edges should be ") +
                    QString::number( eprob * newNodes*(newNodes-1)) +
                    tr("\nThis graph is almost surely connected because: \nprobability > ln(n)/n, that is: \n")
                    + QString::number(eprob)+
                    tr(" bigger than ")+ QString::number(threshold) , "OK",0);

    else
        QMessageBox::information(
                    this,
                    "New Erdos-Renyi Random Network",
                    tr("Random network created. \n")+
                    //tr("\nAverage path length: ") + QString::number(avGraphDistance)+
                    //tr("\nClustering coefficient: ")+QString::number(clucof)+
                    tr("\n\nOn the average, edges should be ")
                    + QString::number(eprob * newNodes*(newNodes-1)) +
                    tr("\nThis graph is almost surely not connected because: \nprobability < ln(n)/n, that is: \n") +
                    QString::number(eprob)+ " smaller than "+ QString::number(threshold) , "OK",0);

    statusMessage( tr("Erdos-Renyi Random Network created. ") ) ;

}







/**
 * @brief MainWindow::slotNetworkRandomScaleFreeDialog
 */
void MainWindow::slotNetworkRandomScaleFreeDialog() {
    qDebug() << "MW::slotNetworkRandomScaleFreeDialog()";
    statusMessage( tr("Generate a random Scale-Free network. "));
    m_randScaleFreeDialog = new DialogRandScaleFree(this);

    connect( m_randScaleFreeDialog, &DialogRandScaleFree::userChoices,
             this, &MainWindow::slotNetworkRandomScaleFree);

    m_randScaleFreeDialog->exec();

}


/**
 * @brief MainWindow::slotNetworkRandomScaleFree
 * @param nodes
 * @param power
 * @param initialNodes
 * @param edgesPerStep
 * @param zeroAppeal
 * @param mode
 */
void MainWindow::slotNetworkRandomScaleFree ( const int &newNodes,
                                          const int &power,
                                          const int &initialNodes,
                                          const int &edgesPerStep,
                                          const float &zeroAppeal,
                                          const QString &mode)
{
    qDebug() << "MW::slotNetworkRandomScaleFree()";

    initApp();

    activeGraph.randomNetScaleFreeCreate( newNodes,
                                          power,
                                          initialNodes,
                                          edgesPerStep,
                                          zeroAppeal,
                                          mode);


    setWindowTitle("Untitled scale-free network");

    //float avGraphDistance=activeGraph.graphDistanceGeodesicAverage();
    //float clucof=activeGraph.clusteringCoefficient();
    QMessageBox::information(this, "New scale-free network",
                             tr("Scale-free random network created.\n")
//                             +tr("\nNodes: ")+ QString::number(nodesSelected)+
//                             tr("\nEdges: ") +  QString::number( edgeCount )
                             //+  tr("\nAverage path length: ") + QString::number(avGraphDistance)
                             //+ tr("\nClustering coefficient: ")+QString::number(clucof)
                             , "OK",0);

    statusMessage( tr("Scale-Free Random Network created. ") );

}



/**
 * @brief MainWindow::slotNetworkRandomSmallWorldDialog
 */
void MainWindow::slotNetworkRandomSmallWorldDialog()
{
    qDebug() << "MW::slotNetworkRandomSmallWorldDialog()";
    statusMessage( tr("Generate a random Small-World network. "));
    m_randSmallWorldDialog = new DialogRandSmallWorld(this);

    connect( m_randSmallWorldDialog, &DialogRandSmallWorld::userChoices,
             this, &MainWindow::slotNetworkRandomSmallWorld);


    m_randSmallWorldDialog->exec();

}


/**
 * @brief MainWindow::slotNetworkRandomSmallWorld
 * @param nodes
 * @param degree
 * @param beta
 * @param mode
 * @param diag
 */
void MainWindow::slotNetworkRandomSmallWorld(const int &newNodes,
                                            const int &degree,
                                            const float &beta,
                                            const QString &mode,
                                            const bool &diag)
{
    Q_UNUSED(diag);
    qDebug() << "MW::slotNetworkRandomSmallWorld()";

    initApp();

    activeGraph.randomNetSmallWorldCreate(newNodes, degree, beta, mode);

    setWindowTitle("Untitled small-world network");

    //float avGraphDistance=activeGraph.graphDistanceGeodesicAverage();
    //float clucof=activeGraph.clusteringCoefficient();
    QMessageBox::information(this, "New Small World network",
                             tr("Small world network created.\n")
//                             +tr("\nNodes: ")+ QString::number(nodeCount)+
//                             tr("\nEdges: ") +  QString::number( edgeCount )
                             //+  tr("\nAverage path length: ") + QString::number(avGraphDistance)
                             //+ tr("\nClustering coefficient: ")+QString::number(clucof)
                             , "OK",0);


    statusMessage( tr("Small World Random Network created. ") );
}





/**
 * @brief MainWindow::slotNetworkRandomRegularDialog
 */
void MainWindow::slotNetworkRandomRegularDialog()
{
    qDebug() << "MW::slotRandomRegularDialog()";
    statusMessage( tr("Generate a d-regular random network. "));
    m_randRegularDialog = new DialogRandRegular(this);

    connect( m_randRegularDialog, &DialogRandRegular::userChoices,
             this, &MainWindow::slotNetworkRandomRegular);

    m_randRegularDialog->exec();

}

/**
 * @brief MainWindow::slotNetworkRandomRegular
 * Creates a pseudo-random k-regular network where every node has the same degree
 */
void MainWindow::slotNetworkRandomRegular(const int &newNodes, const int &degree,
                                          const QString &mode, const bool &diag){


    initApp();

    activeGraph.randomNetRegularCreate (newNodes,degree, mode, diag);

    setWindowTitle("Untitled d-regular network");

    //float avGraphDistance=activeGraph.graphDistanceGeodesicAverage();
    //float clucof=activeGraph.clusteringCoefficient();
    QMessageBox::information(this, "New d-Regular network",
                             tr("d-Regular network created.\n")
//                             +tr("\nNodes: ")+ QString::number(nodeCount)+
//                             tr("\nEdges: ") +  QString::number( edgeCount )
                             //+  tr("\nAverage path length: ") + QString::number(avGraphDistance)
                             //+ tr("\nClustering coefficient: ")+QString::number(clucof)
                             , "OK",0);



    statusMessage( tr( "d-regular network created. " ) );

}




void MainWindow::slotNetworkRandomGaussian(){

}


/**
 * @brief MainWindow::slotNetworkRandomRingLattice
 * Creates a lattice network, i.e. a connected network where every node
    has the same degree and is connected with its neighborhood.
 */
void MainWindow::slotNetworkRandomRingLattice(){
    bool ok;
    statusMessage( "You have selected to create a ring lattice network. ");
    int newNodes=( QInputDialog::getInt(
                       this,
                       tr("Create ring lattice"),
                       tr("This will create a ring lattice network, "
                          "where each node has degree d:\n d/2 edges to the right "
                          "and d/2 to the left.\n"
                          "Please enter the number of nodes you want:"),
                       100, 4, maxNodes, 1, &ok ) ) ;
    if (!ok) {
        statusMessage( "You did not enter an integer. Aborting.");
        return;
    }
    int degree = QInputDialog::getInt(
                this,
                tr("Create ring lattice..."),
                tr("Now, enter an even number d. \n"
                   "This is the total number of edges each new node will have:"),
                2, 2, newNodes-1, 2, &ok);
    if ( (degree% 2)==1 ) {
        QMessageBox::critical(this, "Error",tr(" Sorry. I cannot create such a network. "
                                               "Degree must be even number"), "OK",0);
        return;
    }


    initApp();

    activeGraph.randomNetRingLatticeCreate(newNodes, degree, true );


    setWindowTitle("Untitled ring-lattice network");
    //float avGraphDistance=activeGraph.graphDistanceGeodesicAverage();
    //float clucof=activeGraph.clusteringCoefficient();
    QMessageBox::information(this, "New Ring Lattice",
                             tr("Ring lattice network created.\n")
//                             +tr("\nNodes: ")+ QString::number(activeNodes())+
//                             tr("\nEdges: ")+  QString::number( activeEdges() )
                             // + tr("\nAverage path length: ") + QString::number(avGraphDistance)
                             //+ tr("\nClustering coefficient: ")+QString::number(clucof)
                             , "OK",0);

    statusMessage( tr("Ring lattice random network created: " ));
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
void MainWindow::slotNetworkChanged(const int &graphStatus,
                                    const bool &undirected,
                                    const int &vertices, const int &edges,
                                    const float &density){
    qDebug()<<"MW::slotNetworkChanged()"
           <<"graphStatus" << graphStatus
             <<"undirected" << undirected
            <<"vertices" << vertices
           <<"edges" <<edges
          << "density"<<density;

    if (graphStatus) {
        networkSave->setIcon(QIcon(":/images/save.png"));
        networkSave->setEnabled(true);
    }
    rightPanelNodesLCD->display(vertices);
    if ( undirected ) {
        rightPanelEdgesLCD->setStatusTip(tr("Shows the total number of undirected edges in the network."));
        rightPanelEdgesLCD->setToolTip(tr("The total number of undirected edges in the network."));
        rightPanelNetworkTypeLabel->setStatusTip(tr("Undirected data mode. Toggle the menu option Edit -> Edges -> Undirected Edges to change it"));
        rightPanelNetworkTypeLabel->setToolTip(tr("The loaded network, if any, is undirected and \n"
                                    "any edge you add between nodes will be undirected.\n"
                                    "If you want to work with directed edges and/or \n"
                                    "transform the loaded network (if any) to directed \n"
                                    "disable the option Edit -> Edges -> Undirected \n"
                                    "or press CTRL+E+U"));
        rightPanelNetworkTypeLabel->setWhatsThis(tr("The loaded network, if any, is undirected and \n"
                                    "any edge you add between nodes will be undirected.\n"
                                    "If you want to work with directed edges and/or \n"
                                    "transform the loaded network (if any) to directed \n"
                                    "disable the option Edit -> Edges -> Undirected \n"
                                    "or press CTRL+E+U"));


        if (toolBoxEditEdgeModeSelect->currentIndex()==0) {
            toolBoxEditEdgeModeSelect->setCurrentIndex(1);
        }
        rightPanelNetworkTypeLabel-> setText ("Network Type: Undirected");
        rightPanelEdgesLabel->setText(tr("Total Edges"));
        rightPanelSelectedEdgesLabel->setText( tr("Selected Edges") );
        editEdgeUndirectedAllAct->setChecked(true);
    }
    else {
        rightPanelEdgesLCD->setStatusTip(tr("Shows the total number of directed edges in the network."));
        rightPanelEdgesLCD->setToolTip(tr("The total number of directed edges in the network."));
        rightPanelNetworkTypeLabel->setStatusTip(tr("Directed data mode. Toggle the menu option Edit -> Edges -> Undirected Edges to change it"));
        rightPanelNetworkTypeLabel->setToolTip(tr("The loaded network, if any, is directed and \n"
                                    "any link you add between nodes will be a directed arc.\n"
                                    "If you want to work with undirected edges and/or \n"
                                    "transform the loaded network (if any) to undirected \n"
                                    "enable the option Edit -> Edges -> Undirected \n"
                                    "or press CTRL+E+U"));
        rightPanelNetworkTypeLabel->setWhatsThis(tr("The loaded network, if any, is directed and \n"
                                    "any link you add between nodes will be a directed arc.\n"
                                    "If you want to work with undirected edges and/or \n"
                                    "transform the loaded network (if any) to undirected \n"
                                    "enable the option Edit -> Edges -> Undirected \n"
                                    "or press CTRL+E+U"));

        rightPanelNetworkTypeLabel-> setText ("Network Type: Directed");
        if (toolBoxEditEdgeModeSelect->currentIndex()==1) {
            toolBoxEditEdgeModeSelect->setCurrentIndex(0);
        }
        rightPanelEdgesLabel->setText(tr("Total Arcs"));
        rightPanelSelectedEdgesLabel->setText( tr("Selected Arcs")  );
        editEdgeUndirectedAllAct->setChecked(false);
    }
    rightPanelEdgesLCD->display(edges);
    rightPanelDensityLCD->display( density );
}








/**
 * @brief MainWindow::slotEditOpenContextMenu
 * Popups a context menu with some options when the user right-clicks on the scene
 * @param mPos
 */
void MainWindow::slotEditOpenContextMenu( const QPointF &mPos) {
    Q_UNUSED(mPos);
    QMenu *contextMenu = new QMenu(" Menu",this);
    Q_CHECK_PTR( contextMenu );  //displays "out of memory" if needed

    int nodesSelected = activeGraph.graphSelectedVerticesCount();

    contextMenu -> addAction( "## Selected nodes: "
                              + QString::number(  nodesSelected ) + " ##  ");

    contextMenu -> addSeparator();

    if (nodesSelected > 0) {
        contextMenu -> addAction(editNodePropertiesAct );
        contextMenu -> addSeparator();
        contextMenu -> addAction(editNodeRemoveAct );
        if (nodesSelected > 1 ){
            editNodeRemoveAct->setText(tr("Remove ")
                                       + QString::number(nodesSelected)
                                       + tr(" nodes"));
            contextMenu -> addSeparator();
            contextMenu -> addAction(editNodeSelectedToCliqueAct);
            contextMenu -> addAction(editNodeSelectedToStarAct);
            contextMenu -> addAction(editNodeSelectedToCycleAct);
            contextMenu -> addAction(editNodeSelectedToLineAct);

        }
        else {
            editNodeRemoveAct->setText(tr("Remove ")
                                       + QString::number(nodesSelected)
                                       + tr(" node"));
        }
        contextMenu -> addSeparator();
    }

    contextMenu -> addAction( editNodeAddAct );
    contextMenu -> addSeparator();
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
    options -> addAction (editEdgeColorAllAct  );
    options -> addSeparator();
    options -> addAction (changeBackColorAct  );
    options -> addAction (backgroundImageAct  );

    //QCursor::pos() is good only for menus not related with node coordinates
    contextMenu -> exec(QCursor::pos() );
    delete  contextMenu;
}




/**
 * @brief MainWindow::slotEditClickOnEmptySpace
 * Called from GW when the user clicks on empty space.
 */
void MainWindow::slotEditClickOnEmptySpace(const QPointF &p) {
    rightPanelClickedNodeLCD->display (0);
    rightPanelClickedNodeInDegreeLCD->display (0);
    rightPanelClickedNodeOutDegreeLCD->display (0);
    rightPanelClickedNodeClucofLCD->display(0);
    activeGraph.vertexClickedSet(0);
    activeGraph.edgeClickedSet(0,0);
    statusMessage( tr("Position (%1,%2): Nothing here. Cleared any selection. Double-click to create a new node." )
                   .arg(p.x())
                   .arg(p.y())  );
}



/**
 * @brief MainWindow::slotEditNodeSelectAll
 */
void MainWindow::slotEditNodeSelectAll(){
    qDebug() << "MainWindow::slotEditNodeSelectAll()";
    graphicsWidget->selectAll();
    statusMessage( tr("Selected nodes: %1")
                   .arg( activeGraph.graphSelectedVerticesCount()  ) );

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
 * @brief MainWindow::slotEditNodePosition
 * Called from GraphicsWidget when a node moves to update vertex coordinates
 * in Graph
 * @param nodeNumber
 * @param x
 * @param y
 */
void MainWindow::slotEditNodePosition(const int &nodeNumber,
                                  const int &x, const int &y){
    qDebug("MW::slotEditNodePosition() for %i with x %i and y %i", nodeNumber, x, y);
    activeGraph.vertexPosSet(nodeNumber, x, y);
    if (!activeGraph.graphSaved()) {
        networkSave->setIcon(QIcon(":/images/save.png"));
        networkSave->setEnabled(true);
    }
}


/**
 * @brief MainWindow::slotEditNodeAdd
 * Calls Graph::vertexCreate method to add a new RANDOM node into the activeGraph.
 * Called when "Add Node" button is clicked on the Main Window.
 */
void MainWindow::slotEditNodeAdd() {
    qDebug() << "MW::slotEditNodeAdd() - calling Graph::vertexCreateAtPosRandom ";
    activeGraph.vertexCreateAtPosRandom(true);
    statusMessage( tr("New random positioned node (numbered %1) added.")
                   .arg(activeGraph.vertexNumberMax())  );
}



/**
 * @brief MainWindow::slotEditNodeAddWithMouse
 * Called by GW when user double-clicks at p to add a new node
 * Calls Graph::vertexCreateAtPos() method to add the new vertex
 * @param p
 */
void MainWindow::slotEditNodeAddWithMouse( const QPointF &p) {
    qDebug()<< "MW::slotEditNodeAddWithMouse() - "
               "Calling activeGraph::vertexCreateAtPos()";
    activeGraph.vertexCreateAtPos(p);
    statusMessage(  tr("New node (numbered %1) added at position (%2,%3)")
                   .arg(activeGraph.vertexNumberMax())
                   .arg( p.x() )
                   .arg( p.y() )
                   ) ;
}



/**
 * @brief MainWindow::slotEditNodeFind
 * Calls GW::setMarkedNode() to find a node by its number or label.
 * The node is then marked.
 */
void MainWindow::slotEditNodeFind(){
    qDebug() << "MW::slotEditNodeFind()";
    if ( !activeNodes() ) {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
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
 * If user has clicked on a node (signaled from GW or set by another function)
 * it deletes it
 * Else it asks for a nodeNumber to remove. The nodeNumber is doomedJim.
 * Called from nodeContextMenu
 */
void MainWindow::slotEditNodeRemove() {
    qDebug() << "MW::slotEditNodeRemove()";
    if ( !activeNodes() )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    if (activeGraph.relations() > 1){
        QMessageBox::critical(
                    this, "Error",
                    tr("Cannot remove node! \n"
                       "This a network with more than 1 relations. If you remove "
                       "a node from the active relation, and then ask me to go "
                       "to the previous or the next relation, then I would crash "
                       "because I would try to display edges from a deleted node."
                       "You cannot remove nodes in multirelational networks."),
                    "OK",0);
        statusMessage( tr("Nothing to remove.")  );
        return;
    }

    // if there are already multiple nodes selected, erase them
    int nodesSelected = activeGraph.graphSelectedVerticesCount();
    if ( nodesSelected > 0) {
        int removeCounter = 0;
        qDebug() << "MW::removeNode() multiple selected to remove";
        foreach (int nodeNumber, activeGraph.graphSelectedVertices() ) {
               activeGraph.vertexRemove(nodeNumber);
               ++removeCounter ;
        }
        editNodeRemoveAct->setText(tr("Remove Node"));
        statusMessage( tr("Removed ") + nodesSelected + tr(" nodes. Ready. ") );
    }

    else {
        int doomedJim=-1, min=-1, max=-1;
        bool ok=false;
        min = activeGraph.vertexNumberMin();
        max = activeGraph.vertexNumberMax();
        qDebug("MW: min is %i and max is %i", min, max);
        if (min==-1 || max==-1 ) {
            qDebug("ERROR in finding min max nodeNumbers. Abort");
            return;
        }
        else  {
            doomedJim =  QInputDialog::getInt(
                        this,
                        tr("Remove node"),
                        tr("Choose a node to remove between ("
                           + QString::number(min).toLatin1()+"..."+
                           QString::number(max).toLatin1()+"):"),min, 1, max, 1, &ok);
            if (!ok) {
                statusMessage( "Remove node operation cancelled." );
                return;
            }
        }
        qDebug ("MW: removing vertex with number %i from Graph", doomedJim);
        activeGraph.vertexRemove(doomedJim);
        qDebug("MW: removeNode() completed. Node %i removed completely.",doomedJim);
        statusMessage( tr("Node removed completely. Ready. ") );
    }
}



/**
 * @brief MainWindow::slotEditNodePropertiesDialog
 * Reads values from selected nodes
 * then open Node Properties dialog
 */
void MainWindow::slotEditNodePropertiesDialog() {

    qDebug() << "MW::slotEditNodePropertiesDialog()";
    if ( !activeNodes() )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    int min=-1, max=-1, size = appSettings["initNodeSize"].toInt(0, 10);
    int nodeNumber = 0;
    int selectedNodesCount = activeGraph.graphSelectedVerticesCount();
    QColor color = QColor(appSettings["initNodeColor"]);
    QString shape= appSettings["initNodeShape"];
    QString label="";
    bool ok=false;


    if ( selectedNodesCount  == 0) {
        min = activeGraph.vertexNumberMin();
        max = activeGraph.vertexNumberMax();
        qDebug() << "MW::slotEditNodePropertiesDialog() - no node selected"
                    << "min node number " << min
                       << "max node number " << max
                          << "opening inputdialog";
        if (min==-1 || max==-1 ) {
            qDebug("ERROR in finding min max nodeNumbers. Abort");
            return;
        }

        nodeNumber =  QInputDialog::getInt(
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
        foreach (nodeNumber, activeGraph.graphSelectedVertices() ) {
            qDebug() << "MW::slotEditNodePropertiesDialog() "
                        "changing properties for selected node "
                     << nodeNumber ;
            if ( selectedNodesCount > 1 ) {
                color = activeGraph.vertexColor( nodeNumber );
                shape = activeGraph.vertexShape( nodeNumber);
                size = activeGraph.vertexSize ( nodeNumber);
            }
            else {
                label = activeGraph.vertexLabel( nodeNumber );
                color = activeGraph.vertexColor( nodeNumber );
                shape = activeGraph.vertexShape( nodeNumber);
                size = activeGraph.vertexSize ( nodeNumber);
            }
        }
    }

    //@todo add some grouping function here?

    m_nodeEditDialog = new DialogNodeEdit(this, label, size, color, shape) ;

    connect( m_nodeEditDialog, &DialogNodeEdit::userChoices,
             this, &MainWindow::slotEditNodeProperties );

    m_nodeEditDialog->exec();

    statusMessage( tr("Node properties dialog opened. Ready. ") );
}


/**
 * @brief MainWindow::slotEditNodeProperties
 * Applies new (user-defined) values to all selected nodes
 * Called on exit from DialogNodeEdit
 * @param label
 * @param size
 * @param value
 * @param color
 * @param shape
 */
void MainWindow::slotEditNodeProperties( const QString label, const int size,
                                     const QString value, const QColor color,
                                     const QString shape) {

    int selectedNodesCount = activeGraph.graphSelectedVerticesCount();

    qDebug()<< "MW::slotEditNodeProperties() - new properties: "
            << " label " << label
            << " size " << size
            << "value " << value
            << " color " << color
            << " shape " << shape
               << " vertexClicked " <<activeGraph.vertexClicked()
                  << " selectedNodesCount " << selectedNodesCount;

    if ( selectedNodesCount == 0 && activeGraph.vertexClicked() != 0) {
        // no node selected but user entered a node number in a dialog
        if ( label !="" && appSettings["initNodeLabelsVisibility"] != "true")
            slotOptionsNodeLabelsVisibility(true);
        qDebug()<< "MW::slotEditNodeProperties() - updating label ";
        activeGraph.vertexLabelSet( activeGraph.vertexClicked(), label );
        qDebug()<< "MW::slotEditNodeProperties() - updating color ";
        activeGraph.vertexColorSet( activeGraph.vertexClicked(), color.name());
        qDebug()<< "MW::slotEditNodeProperties() - updating size ";
        activeGraph.vertexSizeSet( activeGraph.vertexClicked(), size);
        qDebug()<< "MW::slotEditNodeProperties() - updating shape ";
        activeGraph.vertexShapeSet( activeGraph.vertexClicked(), shape);
    }
    else {
        //some nodes are selected
        int nodeNumber = 0;
        foreach (nodeNumber, activeGraph.graphSelectedVertices() ) {
            qDebug()<< "MW::slotEditNodeProperties() - node " << nodeNumber;
            qDebug()<< "MW::slotEditNodeProperties() - updating label ";
            if ( selectedNodesCount > 1 )
            {
                activeGraph.vertexLabelSet(
                            nodeNumber,
                            label + QString::number(nodeNumber)
                            );
            }
            else
                activeGraph.vertexLabelSet( nodeNumber, label );

            if ( label !="" && appSettings["initNodeLabelsVisibility"] != "true")
                slotOptionsNodeLabelsVisibility(true);


            qDebug()<< "MW::slotEditNodeProperties() - updating color ";
            activeGraph.vertexColorSet( nodeNumber, color.name());
            qDebug()<< "MW::slotEditNodeProperties() - updating size ";
            activeGraph.vertexSizeSet(nodeNumber,size);
            qDebug()<< "MW::slotEditNodeProperties() - updating shape ";
            activeGraph.vertexShapeSet( nodeNumber, shape);
        }
    }

    statusMessage( tr("Ready. "));
}



/**
 * @brief Creates a complete subgraph (clique) from selected nodes.
 * Calls Graph::verticesSelectedCreateClique()
 */
void MainWindow::slotEditNodeSelectedToClique () {
    qDebug() << "MW::slotEditNodeSelectedToClique()";
    if ( !activeNodes() )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    int selectedNodesCount = activeGraph.graphSelectedVerticesCount();

    if ( selectedNodesCount == 0 ) {
        slotHelpMessageToUser(USER_MSG_INFO,tr("No nodes selected."),
                              tr("Cannot create new clique. No nodes are selected."),
                              tr("Select some nodes first.")
                              );
        return;
    }

    activeGraph.verticesCreateSubgraph(QList<int> (), SUBGRAPH_CLIQUE);

    slotHelpMessageToUser(USER_MSG_INFO,tr("Clique created."),
                          tr("A new clique has been created from ") + QString::number(selectedNodesCount)
                          + tr(" nodes")
                          );
}



/**
 * @brief Creates a star subgraph from selected nodes.
 * User must choose a central actor.
 * Calls Graph::slotEditNodeSelectedToStar()
 */
void MainWindow::slotEditNodeSelectedToStar() {
    qDebug() << "MW::slotEditNodeSelectedToStar()";
    if ( !activeNodes() )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    int selectedNodesCount = activeGraph.graphSelectedVerticesCount();

    if ( selectedNodesCount == 0 ) {
        slotHelpMessageToUser(USER_MSG_INFO,tr("No nodes selected."),
                              tr("Cannot create star subgraph. No nodes are selected."),
                              tr("Select some nodes first.")
                              );
        return;
    }
    int center;
    bool ok=false;

    int min = activeGraph.graphSelectedVerticesMin();
    int max = activeGraph.graphSelectedVerticesMax();
    center=QInputDialog::getInt(
                this,
                "Create star subgraph",
                tr("To create a star subgraph from selected nodes, \n"
                   "enter the number of the central actor ("
                   +QString::number(min).toLatin1()+"..."
                   +QString::number(max).toLatin1()+"):"), min, 1, max , 1, &ok ) ;
    if (!ok) {
        statusMessage( "Create star subgraph cancelled." );
        return;
    }

    activeGraph.verticesCreateSubgraph(QList<int> (), SUBGRAPH_STAR,center);

    slotHelpMessageToUser(USER_MSG_INFO,tr("Star subgraph created."),
                          tr("A new star subgraph has been created from ") +
                          QString::number( selectedNodesCount )
                          + tr(" nodes")
                          );
}



/**
 * @brief Creates a cycle subgraph from selected nodes.
 * Calls Graph::verticesSelectedCreateCycle()
 */
void MainWindow::slotEditNodeSelectedToCycle() {
    qDebug() << "MW::slotEditNodeSelectedToCycle()";
    if ( !activeNodes() )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    int selectedNodesCount = activeGraph.graphSelectedVerticesCount();

    if ( selectedNodesCount == 0 ) {
        slotHelpMessageToUser(USER_MSG_INFO,tr("No nodes selected."),
                              tr("Cannot create cycle subgraph. No nodes are selected."),
                              tr("Select some nodes first.")
                              );
        return;
    }

    activeGraph.verticesCreateSubgraph(QList<int> (),SUBGRAPH_CYCLE);

    slotHelpMessageToUser(USER_MSG_INFO,tr("Cycle subgraph created."),
                          tr("A new cycle subgraph has been created from ")
                          + QString::number( selectedNodesCount )
                          + tr(" nodes")
                          );
}



/**
 * @brief Creates a line subgraph from selected nodes.
 * Calls Graph::verticesSelectedCreateLine()
 */
void MainWindow::slotEditNodeSelectedToLine() {
    qDebug() << "MW::slotEditNodeSelectedToLine()";
    if ( !activeNodes() )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    int selectedNodesCount = activeGraph.graphSelectedVerticesCount();

    if ( selectedNodesCount == 0 ) {
        slotHelpMessageToUser(USER_MSG_INFO,tr("No nodes selected."),
                              tr("Cannot create line subgraph. No nodes are selected."),
                              tr("Select some nodes first.")
                              );
        return;
    }

    activeGraph.verticesCreateSubgraph(QList<int> (),SUBGRAPH_LINE);

    slotHelpMessageToUser(USER_MSG_INFO,tr("Line subgraph created."),
                          tr("A new line subgraph has been created from ")
                          + QString::number( selectedNodesCount )
                          + tr(" nodes")
                          );
}



/**
 * @brief MainWindow::slotEditNodeColorAll
 * Changes the color of all nodes to parameter color
 * Calls  activeGraph.vertexColorAllSet to do the work
 * If parameter color is invalid, opens a QColorDialog to
 * select a new node color for all nodes.
 * Called from Settings Dialog and Edit menu option
 * @param color
 */
void MainWindow::slotEditNodeColorAll(QColor color){
    if (!color.isValid()) {
        color = QColorDialog::getColor( QColor ( appSettings["initNodeColor"] ),
                                        this,
                                               "Change the color of all nodes" );
    }
    if (color.isValid()) {
        appSettings["initNodeColor"] = color.name();
        QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
        qDebug() << "MW::slotEditNodeColorAll() : "
                 << appSettings["initNodeColor"];
        activeGraph.vertexColorAllSet(appSettings["initNodeColor"]);
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
 * Changes the size of nodes to newSize.
 * Calls activeGraph.vertexSizeAllSet to do the work.
 * Called from Edit menu item, DialogSettings
 * If newSize = 0 asks the user a new size for all nodes
 * If normalized = true, changes node sizes according to their plethos
 * @param newSize
 * @param normalized
 */
void MainWindow::slotEditNodeSizeAll(int newSize, const bool &normalized) {
    Q_UNUSED(normalized);
    qDebug () << "MW: slotEditNodeSizeAll() - "
                 << " newSize " << newSize ;
    if ( newSize == 0 && !normalized ) {
        bool ok=true;
        newSize = QInputDialog::getInt(
                    this,
                    "Change node size",
                    tr("Select new size for all nodes:"),
                    appSettings["initNodeSize"].toInt(0, 10), 1, 100, 1, &ok );

        if (!ok) {
            statusMessage( "Change node size operation cancelled." );
            return;
        }
    }

    appSettings["initNodeSize"]= QString::number(newSize);

    activeGraph.vertexSizeAllSet(newSize);

    statusMessage(tr("Ready"));
    return;
}






/**
 * @brief MainWindow::slotEditNodeShape
 * If shape == null, prompts the user a list of available node shapes to select.
 * Then changes the shape of all nodes/vertices accordingly.
 * If vertex is non-zero, changes the shape of that node only.
 * Called when user clicks on Edit -> Node > Change all nodes shapes
 * Called from DialogSettings when the user has selected a new default node shape
 * Calls Graph::vertexShapeAllSet(QString)
 * @param shape
 * @param vertex
 */
void MainWindow::slotEditNodeShape(QString shape, const int vertex) {
    qDebug() << "MW::slotEditNodeShape() - vertex " << vertex
             << " (0 means all) - new shape " << shape;

    if (shape==QString::null) {
        bool ok=false;
        QStringList lst;
        lst << "box"<< "circle"<< "diamond"
            << "ellipse"<< "triangle" << "star";
        int curShapeIndex = lst.indexOf(appSettings["initNodeShape"]);
        if ( curShapeIndex == -1 ) {
            curShapeIndex=1;
        }
        shape = QInputDialog::getItem(this,
                                      "Node shape",
                                      "Select a shape for all nodes: ",
                                      lst, curShapeIndex, true, &ok);
        if ( !ok ) {
            //user pressed Cancel
            statusMessage(tr("Change node shapes aborted."));
            return;
        }
    }

    if (vertex == 0) { //change all nodes shapes
        activeGraph.vertexShapeAllSet(shape);
        appSettings["initNodeShape"] = shape;
        statusMessage(tr("All shapes have been changed. Ready."));
    }
    else { //only one
       activeGraph.vertexShapeSet( vertex, shape);
       statusMessage(tr("Node shape has been changed. Ready."));
    }
}




/**
 * @brief MainWindow::slotEditNodeNumberSize
 * Changes the size of one or all node numbers.
 * Called from Edit menu option and DialogSettings
 * if newSize=0, asks the user to enter a new node number font size
 * if v1=0, it changes all node numbers
 * @param v1
 * @param newSize
 */
void MainWindow::slotEditNodeNumberSize(int v1, int newSize, const bool prompt) {
    bool ok=false;
    qDebug() << "MW::slotEditNodeNumberSize - newSize " << newSize;
    if (prompt) {
        newSize = QInputDialog::getInt(this, "Change text size",
                                       tr("Change all node numbers size to: (1-16)"),
                                       appSettings["initNodeNumberSize"].toInt(0,10), 1, 16, 1, &ok );
        if (!ok) {
            statusMessage( tr("Change font size: Aborted.") );
            return;
        }
    }
    if (v1) { //change one node number only
        activeGraph.vertexNumberSizeSet(v1, newSize);
    }
    else { //change all
        appSettings["initNodeNumberSize"] = QString::number(newSize);
        activeGraph.vertexNumberSizeSetAll(newSize);
    }
    statusMessage( tr("Changed node numbers size. Ready.") );
}




/**
 * @brief MainWindow::slotEditNodeNumbersColor
 * Changes the color of all nodes' numbers.
 * Called from Edit menu option and Settings dialog.
 * Asks the user to enter a new node number color
 */
void MainWindow::slotEditNodeNumbersColor(QColor color){
    qDebug() << "MW:slotEditNodeNumbersColor() - new color " << color;
    if (!color.isValid()) {
        color = QColorDialog::getColor( QColor ( appSettings["initNodeNumberColor"] ),
                                        this,
                                               "Change the color of all node numbers" );
    }

    if (color.isValid()) {
        QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
        QList<QGraphicsItem *> list= scene->items();
        for (QList<QGraphicsItem *>::iterator it=list.begin(); it!=list.end(); it++) {
            if ( (*it)->type() == TypeNumber) 		{
                NodeNumber *jimNumber = (NodeNumber *) (*it);
                jimNumber->update();
                jimNumber->setDefaultTextColor(color);
            }
        }
        appSettings["initNodeNumberColor"] = color.name();
        activeGraph.vertexNumberColorInit( color.name() );
        QApplication::restoreOverrideCursor();
        statusMessage( tr("Numbers' colors changed. Ready. ")  );
    }
    else {
        // user pressed Cancel
        statusMessage( tr("Invalid color. ") );
    }

}


/**
 * @brief MainWindow::slotEditNodeNumberDistance
 * Changes the distance of one or all node numbers from their nodes.
 * Called from Edit menu option and DialogSettings
 * if newDistance=0, asks the user to enter a new node number distance
 * if v1=0, it changes all node number distances
 * @param v1
 * @param newDistance
 */
void MainWindow::slotEditNodeNumberDistance(int v1, int newDistance) {
    bool ok=false;
    qDebug() << "MW::slotEditNodeNumberDistance - newSize " << newDistance;
    if (!newDistance) {
        newDistance = QInputDialog::getInt(
                    this, "Change node number distance",
                    tr("Change all node numbers distance from their nodes to: (1-16)"),
                    appSettings["initNodeNumberDistance"].toInt(0,10), 1, 16, 1, &ok );
        if (!ok) {
            statusMessage( tr("Change node number distance aborted.") );
            return;
        }
    }
    if (v1) { //change one node number distance only
        activeGraph.vertexNumberDistanceSet(v1, newDistance);
    }
    else { //change all
        appSettings["initNodeNumberDistance"] = QString::number(newDistance);
        activeGraph.vertexNumberDistanceSetAll(newDistance);
    }
    statusMessage( tr("Changed node number distance. Ready.") );
}



/**
 * @brief MainWindow::slotEditNodeLabelSize
 * Changes the size of one or all node Labels.
 * Called from Edit menu option and DialogSettings
 * if newSize=0, asks the user to enter a new node Label font size
 * if v1=0, it changes all node Labels
 * @param v1
 * @param newSize
 */
void MainWindow::slotEditNodeLabelSize(int v1, int newSize) {
    bool ok=false;
    qDebug() << "MW::slotEditNodeLabelSize - newSize " << newSize;
    if (!newSize) {
        newSize = QInputDialog::getInt(this, "Change text size",
                                       tr("Change all node labels text size to: (1-16)"),
                                       appSettings["initNodeLabelSize"].toInt(0,10), 1, 32, 1, &ok );
        if (!ok) {
            statusMessage( tr("Change font size: Aborted.") );
            return;
        }
    }
    if (v1) { //change one node Label only
        activeGraph.vertexLabelSizeSet(v1, newSize);
    }
    else { //change all
        appSettings["initNodeLabelSize"] = QString::number(newSize);
        activeGraph.vertexLabelSizeAllSet(newSize);
    }
    statusMessage( tr("Changed node label size. Ready.") );
}







/**
 * @brief MainWindow::slotEditNodeLabelsColor
 * Changes the color of all nodes' labels.
 * Asks the user to enter a new node label color
 */
void MainWindow::slotEditNodeLabelsColor(QColor color){
    qDebug() << "MW:slotEditNodeNumbersColor() - new color " << color;
    if (!color.isValid()) {
        color = QColorDialog::getColor( QColor ( appSettings["initNodeLabelColor"] ),
                                        this,
                                               "Change the color of all node labels" );
    }
    if (color.isValid()) {
        QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
        activeGraph.vertexLabelColorAllSet(color.name());
        appSettings["initNodeLabelColor"] = color.name();
        optionsNodeLabelsVisibilityAct->setChecked(true);
        QApplication::restoreOverrideCursor();
        statusMessage( tr("Label colors changed. Ready. ")  );
    }
    else {
        // user pressed Cancel
        statusMessage( tr("Invalid color. ") );
    }
}




/**
 * @brief MainWindow::slotEditNodeLabelDistance
 * Changes the distance of one or all node label from their nodes.
 * Called from Edit menu option and DialogSettings
 * if newDistance=0, asks the user to enter a new node label distance
 * if v1=0, it changes all node label distances
 * @param v1
 * @param newDistance
 */
void MainWindow::slotEditNodeLabelDistance(int v1, int newDistance) {
    bool ok=false;
    qDebug() << "MW::slotEditNodeLabelDistance - newSize " << newDistance;
    if (!newDistance) {
        newDistance = QInputDialog::getInt(
                    this, "Change node label distance",
                    tr("Change all node labels distance from their nodes to: (1-16)"),
                    appSettings["initNodeLabelDistance"].toInt(0,10), 1, 16, 1, &ok );
        if (!ok) {
            statusMessage( tr("Change node label distance aborted.") );
            return;
        }
    }
    if (v1) { //change one node label distance only
        activeGraph.vertexLabelDistanceSet(v1, newDistance);
    }
    else { //change all
        appSettings["initNodeLabelDistance"] = QString::number(newDistance);
        activeGraph.vertexLabelDistanceAllSet(newDistance);
    }
    statusMessage( tr("Changed node label distance. Ready.") );
}



/**
 * @brief MainWindow::slotEditNodeOpenContextMenu
 * Called from GW when the user has right-clicked on a node
 * Opens a node context menu with some options when the user right-clicks on a node
 */
void MainWindow::slotEditNodeOpenContextMenu() {

    qDebug("MW: slotEditNodeOpenContextMenu() for node %i at %i, %i",
           activeGraph.vertexClicked(), QCursor::pos().x(), QCursor::pos().y());

    QMenu *nodeContextMenu = new QMenu(QString::number( activeGraph.vertexClicked() ), this);
    Q_CHECK_PTR( nodeContextMenu );  //displays "out of memory" if needed
    int nodesSelected = activeGraph.graphSelectedVerticesCount();
    if ( nodesSelected == 1) {
        nodeContextMenu -> addAction(
                    tr("## NODE ") + QString::number(activeGraph.vertexClicked()) + " ##  "
                    );
    }
    else {
        nodeContextMenu -> addAction(
                    tr("## NODE ") + QString::number(activeGraph.vertexClicked())
                    + " ##  " + tr(" (selected nodes: ")
                    + QString::number ( nodesSelected ) + ")");
    }

    nodeContextMenu -> addSeparator();

    nodeContextMenu -> addAction(editNodePropertiesAct );

    nodeContextMenu -> addSeparator();

    nodeContextMenu -> addAction(editEdgeAddAct);

    nodeContextMenu -> addSeparator();

    nodeContextMenu -> addAction(editNodeRemoveAct );


    nodeContextMenu -> addSeparator();


    //QCursor::pos() is good only for menus not related with node coordinates
    nodeContextMenu -> exec(QCursor::pos() );
    delete  nodeContextMenu;

}



/**
 * @brief MainWindow::slotEditSelectionChanged
 * @param nodes
 * @param edges
 */
void MainWindow::slotEditSelectionChanged(const int &selNodes, const int &selEdges) {
    qDebug()<< "MW::slotEditSelectionChanged()";
    rightPanelSelectedNodesLCD->display(selNodes);
    rightPanelSelectedEdgesLCD->display(selEdges);

    if (selNodes > 1 ){
        editNodeRemoveAct->setText(tr("Remove ")
                                   + QString::number(selNodes)
                                   + tr(" nodes"));
        editNodeSelectedToCliqueAct->setEnabled(true);
        editNodeSelectedToCliqueAct->setText(tr("Create a clique from ")
                                             + QString::number(selNodes)
                                             + tr(" selected nodes"));
        editNodeSelectedToStarAct->setEnabled(true);
        editNodeSelectedToStarAct->setText(tr("Create a star from ")
                                             + QString::number(selNodes)
                                             + tr(" selected nodes"));
        editNodeSelectedToCycleAct->setEnabled(true);
        editNodeSelectedToCycleAct->setText(tr("Create a cycle from ")
                                             + QString::number(selNodes)
                                             + tr(" selected nodes"));
        editNodeSelectedToLineAct->setEnabled(true);
        editNodeSelectedToLineAct->setText(tr("Create a line from ")
                                             + QString::number(selNodes)
                                             + tr(" selected nodes"));
    }
    else {
        editNodeRemoveAct->setText(tr("Remove Node"));
        editNodeSelectedToCliqueAct->setText(tr("Create a clique from selected nodes"));
        editNodeSelectedToCliqueAct->setEnabled(false);
        editNodeSelectedToStarAct->setText(tr("Create a star from selected nodes"));
        editNodeSelectedToStarAct->setEnabled(false);
        editNodeSelectedToCycleAct->setText(tr("Create a cycle from selected nodes"));
        editNodeSelectedToCycleAct->setEnabled(false);
        editNodeSelectedToLineAct->setText(tr("Create a line from selected nodes"));
        editNodeSelectedToLineAct->setEnabled(false);

    }

    statusMessage(  tr("Selected %1 nodes and %2 edges")
                     .arg( selNodes )
                     .arg( selEdges )
                     );
}



/**
 * @brief MainWindow::slotEditNodeInfoStatusBar
 * Called by Graph::userClickedNode() signal, when the user clicks on a node.
 * It displays information about the node on the statusbar.
 * @param jim
 */
void MainWindow::slotEditNodeInfoStatusBar (const int &number,
                                             const QPointF &p,
                                             const QString &label,
                                             const int &inDegree,
                                             const int &outDegree,
                                             const float &clc) {
    qDebug()<<"MW::slotEditNodeInfoStatusBar()";
    rightPanelClickedNodeLCD->display (number);
    rightPanelClickedNodeInDegreeLCD->display (inDegree);
    rightPanelClickedNodeOutDegreeLCD->display (outDegree);
    rightPanelClickedNodeClucofLCD->display(clc);

    if (number!=0)  {

        statusMessage(  QString(tr("Position (%1, %2):  Node %3, label %4 - "
                               "In-Degree: %5, Out-Degree: %6"))
                    .arg( ceil( p.x() ) )
                    .arg( ceil( p.y() )).arg( number )
                    .arg( ( label == "") ? "unset" : label )
                    .arg(inDegree).arg(outDegree) );
    }

}




/**
 * @brief MainWindow::slotEditEdgeInfoStatusBar
 * Called by GW::selectedEdge signal when the user clickes on an edge
 * Displays information about the clicked edge on the statusbar
 * @param edge
 */
void MainWindow::slotEditEdgeInfoStatusBar (const int &v1,
                                            const  int &v2,
                                            const float &weight,
                                            const bool &undirected) {

    rightPanelClickedEdgeSourceLCD->display(v1);
    rightPanelClickedEdgeTargetLCD->display(v2);
    rightPanelClickedEdgeWeightLCD->display(weight);

    if (v1 ==0 || v2 == 0) return;

    if ( undirected ) {
            statusMessage(  QString
                        (tr("Symmetric edge %1 <--> %2 of weight %3 has been selected. "
                                   "Click anywhere else to unselect it."))
                    .arg( v1 ).arg( v2 )
                    .arg( weight )
                            );
            rightPanelClickedEdgeHeaderLabel->setText(tr("Clicked Edge"));
    }
    else {
        statusMessage(  QString(tr("Arc %1 --> %2 of weight %3 has been selected. "
                                   "Click again to unselect it."))
                        .arg( v1 ).arg( v2 )
                        .arg( weight )
                        );
        rightPanelClickedEdgeHeaderLabel->setText(tr("Clicked Directed Edge"));
    }
}




/**
 * @brief MainWindow::slotEditEdgeOpenContextMenu
 * Called by GW::openEdgeMenu when the user right-clicks on an edge
 * Popups a context menu with edge- related options
 */
void MainWindow::slotEditEdgeOpenContextMenu() {
    int source=activeGraph.edgeClicked().v1;
    int target=activeGraph.edgeClicked().v2;
    qDebug("MW: slotEditEdgeOpenContextMenu() for edge %i-%i at %i, %i",source, target, QCursor::pos().x(), QCursor::pos().y());
    QString edgeName=QString::number(source)+QString("->")+QString::number(target);
    //make the menu
    QMenu *edgeContextMenu = new QMenu(edgeName, this);
    edgeContextMenu -> addAction( "## EDGE " + edgeName + " ##  ");
    edgeContextMenu -> addSeparator();
    edgeContextMenu -> addAction( editEdgeRemoveAct );
    edgeContextMenu -> addAction( editEdgeWeightAct );
    edgeContextMenu -> addAction( editEdgeLabelAct );
    edgeContextMenu -> addAction( editEdgeColorAct );
    edgeContextMenu -> exec(QCursor::pos() );
    delete  edgeContextMenu;
}


/**
 * @brief MainWindow::slotEditEdgeAdd
 * Adds a new edge between two nodes specified by the user.
 * Called when user clicks on the MW button/menu item "Add edge"
 */
void MainWindow::slotEditEdgeAdd(){
    qDebug ()<<"MW::slotEditEdgeAdd()";
    if ( !activeNodes() )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    int sourceNode=-1, targetNode=-1;
    float weight=1; 	//weight of this new edge should be one...
    bool ok=false;
    int min=activeGraph.vertexNumberMin();
    int max=activeGraph.vertexNumberMax();

    if (min==max) return;		//if there is only one node -> no edge

    if ( ! activeGraph.vertexClicked() ) {
        sourceNode=QInputDialog::getInt(
                    this,
                    "Create new edge, Step 1",
                    tr("This will draw a new edge between two nodes. \n"
                       "Enter source node ("
                       +QString::number(min).toLatin1()+"..."
                       +QString::number(max).toLatin1()+"):"), min, 1, max , 1, &ok ) ;
        if (!ok) {
            statusMessage( "Add edge operation cancelled." );
            return;
        }
    }
    else
        sourceNode=activeGraph.vertexClicked();

    qDebug ()<<"MW::slotEditEdgeAdd() - sourceNode:" << sourceNode;

    if ( activeGraph.vertexExists(sourceNode) ==-1 ) {
        statusMessage( tr("Aborting. ")  );
        QMessageBox::critical(this,"Error","No such node.", "OK",0);
        qDebug ()<<"MW::slotEditEdgeAdd() - cannot find sourceNode:" << sourceNode;
        return;
    }

    targetNode=QInputDialog::getInt
            (this, "Create new edge, Step 2",
             tr( "Source node:" ) + QString::number( sourceNode )
                + tr(" \nNow enter a target node [")
                + QString::number(min).toLatin1()
                + "..."
                + QString::number(max).toLatin1()+"]:",min, min, max , 1, &ok)     ;
    if (!ok) {
        statusMessage( "Add edge target operation cancelled." );
        return;
    }
    if ( activeGraph.vertexExists(targetNode) ==-1 ) {
        statusMessage( tr("Aborting. ")  );
        QMessageBox::critical(this,"Error","No such node.", "OK",0);
        qDebug ("MW: slotEditEdgeAdd: Cant find targetNode %i",targetNode);
        return;
    }

    weight=QInputDialog::getDouble(
                this, "Create new edge, Step 3",
                tr("Source and target nodes accepted. \n"
                   "Please, enter the weight of new edge: "),1.0, -100.0, 100.0, 1, &ok);
    if (!ok) {
        statusMessage( "Add edge operation cancelled." );
        return;
    }
    //Check if this edge already exists...
    if (activeGraph.edgeExists(sourceNode, targetNode)!=0 ) {
        qDebug("edge exists. Aborting");
        statusMessage( tr("Aborting. ")  );
        QMessageBox::critical(this,"Error","edge already exists.", "OK",0);
        return;
    }

    slotEditEdgeCreate(sourceNode, targetNode, weight);
    statusMessage( tr("Ready. ")  );
}



/**
 * @brief MainWindow::slotEditEdgeCreate
 * helper to slotEditEdgeAdd() above
 * Also called from GW::userMiddleClicked() signal when user creates edges with middle-clicks
 * Calls Graph::edgeCreate method to add the new edge to the active Graph
  * @param source
 * @param target
 * @param weight
 */
void MainWindow::slotEditEdgeCreate (const int &source, const int &target,
                                     const float &weight) {
    qDebug()<< "MW::slotEditEdgeCreate() - edge"
            << source << "->" << target << "weight" << weight
            << "Setting user settings and calling Graph::edgeCreate(...)";
    //int reciprocal=0;
    bool bezier = false;
    activeGraph.edgeCreate(
                source, target, weight,
                appSettings["initEdgeColor"] ,
                ( editEdgeUndirectedAllAct->isChecked() ) ? 2:0,
                ( editEdgeUndirectedAllAct->isChecked() ) ? false :
                 ( (appSettings["initEdgeArrows"] == "true") ? true: false)
            , bezier);

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
    if ( !activeNodes() || activeEdges() ==0 )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_EDGES);
        return;
    }

    int min=0, max=0, sourceNode=-1, targetNode=-1;
    bool ok=false;
    min=activeGraph.vertexNumberMin();
    max=activeGraph.vertexNumberMax();

    if (!activeGraph.edgeClicked().v1 || !activeGraph.edgeClicked().v2 ) {
        sourceNode=QInputDialog::getInt(
                    this,tr("Remove edge"),
                    tr("Source node:  (")+QString::number(min)+
                    "..."+QString::number(max)+"):", min, 1, max , 1, &ok )   ;
        if (!ok) {
            statusMessage( "Remove edge operation cancelled." );
            return;
        }

        targetNode=QInputDialog::getInt(
                    this,
                    tr("Remove edge"),
                    tr("Target node:  (")+QString::number(min)+"..."+
                    QString::number(max)+"):",min, 1, max , 1, &ok )   ;
        if (!ok) {
            statusMessage( "Remove edge operation cancelled." );
            return;
        }
        if ( activeGraph.edgeExists(sourceNode, targetNode)!=0 ) {
            activeGraph.edgeRemove(sourceNode, targetNode);
        }
        else {
            QMessageBox::critical(
                        this,
                        "Remove edge",tr("There is no such edge."), "OK",0);
            statusMessage( tr("There are no nodes yet...")  );
            return;
        }

    }
    else {
        sourceNode = activeGraph.edgeClicked().v1;
        targetNode = activeGraph.edgeClicked().v2;
        activeGraph.edgeRemove(sourceNode, targetNode);

    }
    qDebug()<< "MW::slotEditEdgeRemove() -"
              << "View items now:"<< graphicsWidget->items().size()
               << "Scene items now:"<< scene->items().size();
}









/**
 * @brief MainWindow::slotEditEdgeLabel
 */
void MainWindow::slotEditEdgeLabel(){
    qDebug() << "MW::slotEditEdgeLabel()";
    if ( !activeEdges() )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_EDGES);
        return;
    }

    int sourceNode=-1, targetNode=-1;
    bool ok=false;

    int min=activeGraph.vertexNumberMin();
    int max=activeGraph.vertexNumberMax();

    if (!activeGraph.edgeClicked().v1 || !activeGraph.edgeClicked().v2 )
    {	//no edge clicked. Ask user to define an edge.
        sourceNode=QInputDialog::getInt(this,
                                        "Change edge label",
                                        tr("Select edge source node:  ("+
                                           QString::number(min).toLatin1()+
                                           "..."+QString::number(max).toLatin1()+
                                           "):"), min, 1, max , 1, &ok)   ;
        if (!ok) {
            statusMessage( "Change edge label operation cancelled." );
            return;
        }
        targetNode=QInputDialog::getInt(this,
                                        "Change edge label...",
                                        tr("Select edge target node:  ("+
                                         QString::number(min).toLatin1()+"..." +
                                         QString::number(max).toLatin1()+"):"),
                                        min, 1, max , 1, &ok  )   ;
        if (!ok) {
            statusMessage( "Change edge label operation cancelled." );
            return;
        }

        if ( ! activeGraph.edgeExists (sourceNode, targetNode ) )  {
             statusMessage( tr("There is no such edge. ") );
             QMessageBox::critical(this, "Error",
                                   tr("No edge! \nNo such edge found in current network."), "OK",0);

             return;
        }

    }
    else
    {	//edge has been clicked.
         sourceNode = activeGraph.edgeClicked().v1;
         targetNode = activeGraph.edgeClicked().v2;
    }

    QString label = QInputDialog::getText( this, tr("Change edge label"),
                                          tr("Enter label: ") );

    if ( !label.isEmpty()) {
        qDebug() << "MW::slotEditEdgeLabel() - " << sourceNode << " -> "
                    << targetNode << " new label " << label;
        activeGraph.edgeLabelSet( sourceNode, targetNode, label);
        slotOptionsEdgeLabelsVisibility(true);
        statusMessage( tr("Ready. ")  );
    }
    else {
        statusMessage( tr("Change edge label aborted. ") );
    }
}






/**
 * @brief MainWindow::slotEditEdgeColorAll
 * It changes the color of all edges weighted below threshold to parameter color
 * If color is not valid, it opens a QColorDialog
 * If threshold == RAND_MAX it changes the color of all edges.
 * Called from Edit -> Edges menu option and Settings Dialog.
 * @param color = QColor()
 * @param threshold = RAND_MAX
 */
void MainWindow::slotEditEdgeColorAll(QColor color, const int threshold){
    if (!color.isValid()) {
        QString text;
        if (threshold < RAND_MAX) {
            text = "Change the color of edges weighted < "
                    + QString::number(threshold) ;
        }
        else
            text = "Change the color of all edges" ;
        color = QColorDialog::getColor( appSettings["initEdgeColor"], this,
                                           text);
    }
    if (color.isValid()) {
        qDebug() << "MainWindow::slotEditEdgeColorAll() - new edge color: " << color.name() << " threshold " << threshold;
        QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
        if (threshold < 0 ) {
            appSettings["initEdgeColorNegative"]=color.name();
        }
        else if (threshold == 0 ) {
            appSettings["initEdgeColorZero"]=color.name();
        }
        else {
            appSettings["initEdgeColor"]=color.name();
        }
        activeGraph.edgeColorAllSet(color.name(), threshold );
        QApplication::restoreOverrideCursor();
        statusMessage( tr("Ready. ")  );
    }
    else {
        // user pressed Cancel
        statusMessage( tr("edges color change aborted. ") );
    }
}




/**
 * @brief MainWindow::slotEditEdgeColor
 * Changes the color of the clicked edge.
 * If no edge is clicked, then it asks the user to specify one.
 */
void MainWindow::slotEditEdgeColor(){
    qDebug() << "MW::slotEditEdgeColor()";
    if (  !activeEdges() )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_EDGES);
        return;
    }

    int sourceNode=-1, targetNode=-1;
    bool ok=false;

    int min=activeGraph.vertexNumberMin();
    int max=activeGraph.vertexNumberMax();

    if (!activeGraph.edgeClicked().v1 || !activeGraph.edgeClicked().v2)
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

        if ( ! activeGraph.edgeExists(sourceNode, targetNode ) )  {
             statusMessage( tr("There is no such edge. ") );
             QMessageBox::critical(this, "Error",
                                   tr("No edge! \nNo such edge found in current network."), "OK",0);

             return;
        }

    }
    else
    {	//edge has been clicked.
         sourceNode = activeGraph.edgeClicked().v1;
         targetNode = activeGraph.edgeClicked().v2;
    }
    QString curColor = activeGraph.edgeColor(sourceNode, targetNode);
    if (!QColor(curColor).isValid()) {
        curColor=appSettings["initEdgeColor"];
    }
    QColor color = QColorDialog::getColor(
                curColor, this, tr("Select new color....") );

    if ( color.isValid()) {
        QString newColor=color.name();
        qDebug() << "MW::slotEditEdgeColor() - " << sourceNode << " -> "
                    << targetNode << " newColor "
                 << newColor;
        activeGraph.edgeColorSet( sourceNode, targetNode, newColor);
        statusMessage( tr("Ready. ")  );
    }
    else {
        statusMessage( tr("Change edge color aborted. ") );
    }

}




/**
 * @brief MainWindow::slotEditEdgeWeight
 * Changes the weight of the clicked edge.
 * If no edge is clicked, asks the user to specify an Edge.
 */
void MainWindow::slotEditEdgeWeight(){
    if (  !activeEdges()  )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_EDGES);
        return;
    }

    qDebug("MW::slotEditEdgeWeight()");
    int  sourceNode=-1, targetNode=-1;
    float newWeight=1.0;
    int min=activeGraph.vertexNumberMin();
    int max=activeGraph.vertexNumberMax();

    bool ok=false;
    if ( activeGraph.edgeClicked().v1==0 || activeGraph.edgeClicked().v2==0 ) {
        sourceNode=QInputDialog::getInt(
                    this,
                    "Change edge weight",
                    tr("Select edge source node:  ("+
                       QString::number(min).toLatin1()+"..."+
                       QString::number(max).toLatin1()+"):"),
                    min, 1, max , 1, &ok)   ;
        if (!ok) {
            statusMessage( "Change edge weight operation cancelled." );
            return;
        }

        targetNode=QInputDialog::getInt(
                    this,
                    "Change edge weight...",
                    tr("Select edge target node:  ("+
                       QString::number(min).toLatin1()+"..."+
                       QString::number(max).toLatin1()+"):"),
                    min, 1, max , 1, &ok  )   ;
        if (!ok) {
            statusMessage( "Change edge weight operation cancelled." );
            return;
        }

        qDebug("source %i target %i",sourceNode, targetNode);
    }
    else {
        qDebug() << "MW: slotEditEdgeWeight() - an Edge has already been clicked";
        sourceNode=activeGraph.edgeClicked().v1;
        targetNode=activeGraph.edgeClicked().v2;
        qDebug() << "MW: slotEditEdgeWeight() from "
                 << sourceNode << " to " << targetNode;

    }

    float oldWeight= 0;
    if ( ( oldWeight= activeGraph.edgeWeight(sourceNode, targetNode)) != 0 ) {
        newWeight=(float) QInputDialog::getDouble(
                    this,
                    "Change edge weight...",
                    tr("New edge Weight: "),
                    oldWeight, -100, 100 ,1, &ok ) ;

        if (ok) {
            activeGraph.edgeWeightSet(sourceNode, targetNode, newWeight,
                                      activeGraph.graphUndirected()
                                      );
        }
        else {
            statusMessage(  QString(tr("Change edge weight cancelled."))  );
            return;
        }
    }

}



/**
 * @brief MainWindow::slotEditEdgeSymmetrizeAll
 * Symmetrize the ties between every two connected nodes.
 * If there is an arc from Node A to Node B,
 * then a new arc from Node B to Node is created of the same weight.
 * Thus, all arcs become reciprocal and the network becomes symmetric
 * with a symmetric adjacency matrix
 */
void MainWindow::slotEditEdgeSymmetrizeAll(){
    if ( activeEdges() ==0 )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_EDGES);
        return;
    }
    qDebug("MW: slotEditEdgeSymmetrizeAll() calling graphSymmetrize()");
    activeGraph.graphSymmetrize();
    QMessageBox::information(this,
                             "Symmetrize",
                             tr("All arcs are reciprocal. \n"
                                "The network is symmetric."), "OK",0);
    statusMessage(tr("All arcs are now reciprocal. Thus a symmetric network. Ready."));
}


/**
 * @brief Adds a new symmetric relation with ties only between pairs of nodes
 * who are cocited by others.
 */
void MainWindow::slotEditEdgeSymmetrizeCocitation(){
    if ( activeEdges() ==0 )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_EDGES);
        return;
    }
    qDebug("MW: slotEditEdgeSymmetrizeCocitation() calling graphCocitation()");
    activeGraph.graphCocitation();
    slotHelpMessageToUser(USER_MSG_INFO,tr("New symmetric cocitation relation created."),
                          tr("New cocitation relation created from strong ties"),
                             tr("A new relation \"%1\" has been added to the network. "
                                "by counting cocitation ties only. "
                             "This relation is symmetric. ").arg("Cocitation"));

}





/**
 * @brief MainWindow::slotEditEdgeSymmetrizeStrongTies
 */
void MainWindow::slotEditEdgeSymmetrizeStrongTies(){
    if ( activeEdges() ==0 )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_EDGES);
        return;
    }
    qDebug()<< "MW::slotEditEdgeSymmetrizeStrongTies() - calling graphSymmetrizeStrongTies()";
    int oldRelationsCounter=activeGraph.relations();
    int answer=0;
    if (oldRelationsCounter>0) {
        switch (
        answer=slotHelpMessageToUser(USER_MSG_QUESTION_CUSTOM, tr("Select"),
                              tr("Symmetrize social network by examining strong ties"),
                              tr("This network has multiple relations. "
                                 "Symmetrize by examining reciprocated ties across all relations or just the current relation?"),
                              QMessageBox::NoButton, QMessageBox::NoButton,
                              tr("all relations"), tr("current relation")
                              )
                ){
        case 1:
            activeGraph.graphSymmetrizeStrongTies(true);
            break;
        case 2:
            activeGraph.graphSymmetrizeStrongTies(false);
            break;
        }


    }
    else {
        activeGraph.graphSymmetrizeStrongTies(false);
    }
    slotHelpMessageToUser(USER_MSG_INFO,tr("New symmetric relation created from strong ties"),
                          tr("New relation created from strong ties"),
                             tr("A new relation \"%1\" has been added to the network. "
                                "by counting reciprocated ties only. "
                             "This relation is binary and symmetric. ").arg("Strong Ties"));

}

/**
 * @brief MainWindow::slotEditEdgeUndirectedAll
 * Tranforms all directed arcs to undirected edges.
 * The result is a undirected and symmetric network
 */
void MainWindow::slotEditEdgeUndirectedAll(const bool &toggle){
    qDebug()<<"MW: slotEditEdgeUndirectedAll() - calling Graph::graphUndirectedSet()";
    if (toggle) {
        activeGraph.graphUndirectedSet(toggle);
        optionsEdgeArrowsAct->setChecked(false);
        if (activeEdges() !=0 ) {
            statusMessage(tr("Undirected data mode. "
                               "All existing directed edges transformed to "
                               "undirected. Ready") ) ;

        }
        else {
            statusMessage( tr("Undirected data mode. "
                               "Any edge you add will be undirected. Ready")) ;
        }
    }
    else {
        activeGraph.graphUndirectedSet(toggle);
        optionsEdgeArrowsAct->trigger();
        optionsEdgeArrowsAct->setChecked(true);
        if (activeEdges() !=0 ) {
            statusMessage ( tr("Directed data mode. "
                               "All existing undirected edges transformed to "
                               "directed. Ready")) ;

        }
        else {
            statusMessage ( tr("Directed data mode. "
                               "Any new edge you add will be directed. Ready")) ;
        }
    }

}



/**
 * @brief Toggles between directed and undirected edge mode
  */
void MainWindow::slotEditEdgeMode(const int &mode){
    qDebug()<<"MW: slotEditEdgeMode() - calling Graph::graphUndirectedSet()";
    if (mode==1) {
        activeGraph.graphUndirectedSet(true);
        optionsEdgeArrowsAct->setChecked(false);
        if (activeEdges() !=0 ) {
            statusMessage(tr("Undirected data mode. "
                               "All existing directed edges transformed to "
                               "undirected. Ready") ) ;

        }
        else {
            statusMessage( tr("Undirected data mode. "
                               "Any edge you add will be undirected. Ready")) ;
        }
    }
    else {
        activeGraph.graphUndirectedSet(false);
        optionsEdgeArrowsAct->trigger();
        optionsEdgeArrowsAct->setChecked(true);
        if (activeEdges() !=0 ) {
            statusMessage ( tr("Directed data mode. "
                               "All existing undirected edges transformed to "
                               "directed. Ready")) ;

        }
        else {
            statusMessage ( tr("Directed data mode. "
                               "Any new edge you add will be directed. Ready")) ;
        }
    }

}




/**
*	Filters Nodes by their value   
    TODO slotFilterNodes
*	
*/
void MainWindow::slotFilterNodes(){

    if ( !activeNodes() )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
}

/**
 * @brief MainWindow::slotEditFilterNodesIsolates
 *Calls Graph::vertexIsolatedAllToggle to toggle visibility of isolated vertices
 */
void MainWindow::slotEditFilterNodesIsolates(bool checked){
    Q_UNUSED(checked);
    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    qDebug()<< "MW: slotEditFilterNodesIsolates";
    activeGraph.vertexIsolatedAllToggle( ! editFilterNodesIsolatesAct->isChecked() );
    statusMessage(  tr("Isolate nodes visibility toggled!")  );
}


/**
*	Shows a dialog from where the user may  
*	filter edges according to their weight 
*	All edges weighted more (or less) than the specified weight  will be disabled.
*/ 
void MainWindow::slotEditFilterEdgesByWeightDialog() {
    if ( !activeEdges()  )   {
        statusMessage(  QString(tr("Load a network file first. \nThen you may ask me to compute something!"))  );
        return;
    }
    m_DialogEdgeFilterByWeight.exec() ;
}



/**
 * @brief MainWindow::slotEditFilterEdgesUnilateral
 * @param checked
 * Calls Graph::edgeFilterUnilateral( bool). If bool==true, all unilateral
 * edges are filtered out.
 */
void MainWindow::slotEditFilterEdgesUnilateral(bool checked) {
    Q_UNUSED(checked);
    if ( !activeEdges() && editFilterEdgesUnilateralAct->isChecked() )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_EDGES);
        return;
    }
    if (activeGraph.relations()>1) {

    }
    qDebug()<< "MW::slotEditFilterEdgesUnilateral";
    activeGraph.edgeFilterUnilateral( ! editFilterEdgesUnilateralAct->isChecked() );
    statusMessage(  tr("Unilateral (weak) edges visibility toggled!")  );
}



/**
*	Transforms all nodes to edges
    TODO slotEditTransformNodes2Edges
*/
void MainWindow::slotEditTransformNodes2Edges(){


}









/**
    TODO slotLayoutColorationStrongStructural
*/
void MainWindow::slotLayoutColorationStrongStructural() {
}


/**
    TODO slotLayoutColorationRegular
*/
void MainWindow::slotLayoutColorationRegular() {
}



/**
 * @brief Calls Graph::layoutRandom
 * to reposition all nodes on a random layout
 */
void MainWindow::slotLayoutRandom(){
    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    graphicsWidget->clearGuides();

    activeGraph.layoutRandom();

    statusMessage( tr("Nodes in random positions.") );
}



/**
 * @brief Calls Graph::layoutRadialRandom
 * to reposition all nodes on a radial layout randomly
 */
void MainWindow::slotLayoutRadialRandom(){
    qDebug() << "MainWindow::slotLayoutRadialRandom()";
    if ( !activeNodes() )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    double x0=scene->width()/2.0;
    double y0=scene->height()/2.0;
    double maxRadius=(graphicsWidget->height()/2.0)-50;          //pixels

    slotLayoutGuides(false);

    activeGraph.layoutRadialRandom(x0, y0, maxRadius, true);

    slotLayoutGuides(true);
    statusMessage( tr("Nodes in random concentric circles.") );
}





/**
 * @brief Calls Graph::layoutForceDirectedSpringEmbedder to embed the Eades
 * spring-gravitational model to the network.
 * Called from menu or toolbox checkbox
 */
void MainWindow::slotLayoutSpringEmbedder(){
    qDebug()<< "MW:slotLayoutSpringEmbedder";
    if ( !activeNodes() )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    activeGraph.layoutForceDirectedSpringEmbedder(500);

    statusMessage( tr("Spring-Gravitational (Eades) model embedded.") );
}





/**
 * @brief Calls Graph::layoutForceDirectedFruchtermanReingold to embed
 * the Fruchterman-Reingold model of repelling-attracting forces to the network.
 * Called from menu or toolbox
 */
void MainWindow::slotLayoutFruchterman(){
    qDebug("MW: slotLayoutFruchterman ()");
    if ( !activeNodes() )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    activeGraph.layoutForceDirectedFruchtermanReingold(100);

    statusMessage( tr("Fruchterman & Reingold model embedded.") );
}





/**
 * @brief Calls Graph::layoutForceDirectedKamadaKawai to embed
 * the Kamada-Kawai FDP model to the network.
 */
void MainWindow::slotLayoutKamadaKawai(){
    qDebug()<< "MW::slotLayoutKamadaKawai ()";
    if ( !activeNodes() )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    activeGraph.layoutForceDirectedKamadaKawai(400);

    statusMessage( tr("Kamada & Kawai model embedded.") );
}






/**
 * @brief
 * Checks sender text() to find out who QMenu item was pressed
 * calls slotLayoutRadialByProminenceIndex(QString)
 */
void MainWindow::slotLayoutRadialByProminenceIndex(){
    qDebug() << "MainWindow::slotLayoutRadialByProminenceIndex()";
    if ( !activeNodes() )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    QAction *menuitem=(QAction *) sender();
    QString menuItemText=menuitem->text();
    qDebug() << "MainWindow::slotLayoutRadialByProminenceIndex() - " <<
                "SENDER MENU IS " << menuItemText;

    slotLayoutRadialByProminenceIndex(menuItemText);

}




/**
 * @brief
 * Overloaded - called when user clicks Apply in the Layout toolbox
 * or from slotLayoutRadialByProminenceIndex() when the user click on menu
 * Places all nodes on concentric circles according to their index score.
*  More prominent nodes are closer to the centre of the screen.
 */
void MainWindow::slotLayoutRadialByProminenceIndex(QString choice=""){
        qDebug() << "MainWindow::slotLayoutRadialByProminenceIndex() ";
    if ( !activeNodes() )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    int userChoice = 0;
    QString prominenceIndexName = choice;
    slotLayoutGuides(true);
    if ( prominenceIndexName.contains("Degree Centr") )
        userChoice=1;
    else if ( prominenceIndexName.contains("Closeness Centr") &&
              !prominenceIndexName.contains("IR"))
        userChoice=2;
    else if ( prominenceIndexName.contains("Influence Range Closeness Centrality")  ||
              prominenceIndexName.contains("IR Closeness")
              )
        userChoice=3;
    else if ( prominenceIndexName.contains("Betweenness Centr"))
        userChoice=4;
    else if (prominenceIndexName.contains("Stress Centr"))
        userChoice=5;
    else if (prominenceIndexName.contains("Eccentricity Centr"))
        userChoice=6;
    else if (prominenceIndexName.contains("Power Centr"))
        userChoice=7;
    else if (prominenceIndexName.contains("Information Centr"))
        userChoice=8;
    else if (prominenceIndexName.contains("Eigenvector Centr"))
        userChoice=9;
    else if (prominenceIndexName.contains("Degree Prestige"))
        userChoice=10;
    else if (prominenceIndexName.contains("PageRank Prestige"))
        userChoice=11;
    else if (prominenceIndexName.contains("Proximity Prestige"))
        userChoice=12;

    qDebug() << "MainWindow::slotLayoutRadialByProminenceIndex() "
             << "prominenceIndexName " << prominenceIndexName
                << " userChoice " << userChoice;

    toolBoxLayoutByIndexSelect->setCurrentIndex(userChoice+1);
    toolBoxLayoutByIndexTypeSelect->setCurrentIndex(0);

    bool dropIsolates=false;
    //check if CC was selected and the graph is disconnected.
    if (userChoice == 2 ) {
        int connectedness=activeGraph.graphConnectedness();
        switch ( connectedness ) {
        case 1:
            break;
        case 2:
            break;
        case -1:
            if (! editFilterNodesIsolatesAct->isChecked() ) {
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
            }
            break;

        case -3:
            if (! editFilterNodesIsolatesAct->isChecked() ) {
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
                    }
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
                   tr("Please note that this function is <b>SLOW</b> on large "
                      "networks (n>200), since it will calculate  a (n x n) matrix A with: <br>"
                      "Aii=1+weighted_degree_ni <br>"
                      "Aij=1 if (i,j)=0 <br>"
                      "Aij=1-wij if (i,j)=wij <br>"
                      "Next, it will compute the inverse matrix C of A. "
                      "The computation of the inverse matrix is a CPU intensive function "
                      "although it uses LU decomposition. <br>"
                      "How slow is this? For instance, to compute IC scores of 600 nodes "
                      "on a modern i7 4790K CPU you will need to wait for 2 minutes at least. <br>"
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

    graphicsWidget->clearGuides();

    activeGraph.layoutByProminenceIndex(
                userChoice, 0,
                considerWeights, inverseWeights,
                editFilterNodesIsolatesAct->isChecked() || dropIsolates);

    statusMessage( tr("Nodes in inner circles have higher %1 score. ").arg(prominenceIndexName ) );

}





/**
 * @brief
 * Checks sender text() to find out who QMenu item was pressed
 * and what prominence index was chosen
 * calls slotLayoutLevelByProminenceIndex(QString)
  */
void MainWindow::slotLayoutLevelByProminenceIndex(){
    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    QAction *menuitem=(QAction *) sender();
    QString menuItemText = menuitem->text();
    qDebug() << "MainWindow::slotLayoutLevelByProminenceIndex() - " <<
                "SENDER MENU IS " << menuItemText;

    slotLayoutLevelByProminenceIndex(menuItemText);

}




/**
 * @brief
 * Overloaded - called when user clicks on toolbox options and when
 * the user selects a menu option (called by slotLayoutLevelByProminenceIndex())
 * Repositions all nodes  on different top-down levels according to the
*  chosen prominence index.
* More prominent nodes are closer to the top of the canvas
 */
void MainWindow::slotLayoutLevelByProminenceIndex(QString choice=""){
    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    int userChoice = 0;
    QString prominenceIndexName = choice;
    slotLayoutGuides(true);

    if ( prominenceIndexName.contains("Degree Centr") )
        userChoice=1;
    else if ( prominenceIndexName.contains("Closeness Centr") &&
              !prominenceIndexName.contains("IR"))
        userChoice=2;
    else if ( prominenceIndexName.contains("Influence Range Closeness Centrality") ||
              prominenceIndexName.contains("IR Closeness"))
        userChoice=3;
    else if ( prominenceIndexName.contains("Betweenness Centr"))
        userChoice=4;
    else if (prominenceIndexName.contains("Stress Centr"))
        userChoice=5;
    else if (prominenceIndexName.contains("Eccentricity Centr"))
        userChoice=6;
    else if (prominenceIndexName.contains("Power Centr"))
        userChoice=7;
    else if (prominenceIndexName.contains("Information Centr"))
        userChoice=8;
    else if (prominenceIndexName.contains("Eigenvector Centr"))
        userChoice=9;
    else if (prominenceIndexName.contains("Degree Prestige"))
        userChoice=10;
    else if (prominenceIndexName.contains("PageRank Prestige"))
        userChoice=11;
    else if (prominenceIndexName.contains("Proximity Prestige"))
        userChoice=12;


    qDebug() << "MainWindow::slotLayoutLevelByProminenceIndex() "
             << "prominenceIndexName " << prominenceIndexName
              << " userChoice " << userChoice;

    toolBoxLayoutByIndexSelect->setCurrentIndex(userChoice+1);
    toolBoxLayoutByIndexTypeSelect->setCurrentIndex(1);

    bool dropIsolates=false;
    //check if CC was selected and the graph is disconnected.
    if (userChoice == 2 ) {
        int connectedness=activeGraph.graphConnectedness();
        switch ( connectedness ) {
        case 1:
            break;
        case 2:
            break;
        case -1:
            if (! editFilterNodesIsolatesAct->isChecked() ) {
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
            }
            break;

        case -3:
            if (! editFilterNodesIsolatesAct->isChecked() ) {
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
            }
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
                   tr("Please note that this function is <b>SLOW</b> on large "
                      "networks (n>200), since it will calculate  a (n x n) matrix A with: <br>"
                      "Aii=1+weighted_degree_ni <br>"
                      "Aij=1 if (i,j)=0 <br>"
                      "Aij=1-wij if (i,j)=wij <br>"
                      "Next, it will compute the inverse matrix C of A. "
                      "The computation of the inverse matrix is a CPU intensive function "
                      "although it uses LU decomposition. <br>"
                      "How slow is this? For instance, to compute IC scores of 600 nodes "
                      "on a modern i7 4790K CPU you will need to wait for 2 minutes at least. <br>"
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

    graphicsWidget->clearGuides();

    activeGraph.layoutByProminenceIndex(
                userChoice, 1,
                considerWeights, inverseWeights,
                editFilterNodesIsolatesAct->isChecked() || dropIsolates);


    statusMessage( tr("Nodes in upper levels have higher %1 score. ").arg(prominenceIndexName ) );

}






/**
 * @brief
 * Checks sender text() to find out who QMenu item was pressed
 * and what prominence index was chosen
 * calls slotLayoutNodeSizeByProminenceIndex(QString)
  */
void MainWindow::slotLayoutNodeSizeByProminenceIndex(){
    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    QAction *menuitem=(QAction *) sender();
    QString menuItemText = menuitem->text();
    qDebug() << "MainWindow::slotLayoutNodeSizeByProminenceIndex() - " <<
                "SENDER MENU IS " << menuItemText;

    slotLayoutNodeSizeByProminenceIndex(menuItemText);

}


/**
 * @brief
 * Calls Graph::layoutByProminenceIndex(2) to apply a layout model
 * where the size of each node follows its prominence score
  * Called when selectbox changes in the toolbox
 */
void MainWindow::slotLayoutNodeSizeByProminenceIndex(QString choice=""){
        qDebug() << "MainWindow::slotLayoutNodeSizeByProminenceIndex() ";
    if ( !activeNodes() )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    int userChoice = 0;
    QString prominenceIndexName = choice;

    if ( prominenceIndexName.contains("Degree Centr") )
        userChoice=1;
    else if ( prominenceIndexName.contains("Closeness Centr") &&
              !prominenceIndexName.contains("IR"))
        userChoice=2;
    else if ( prominenceIndexName.contains("Influence Range Closeness Centrality") ||
              prominenceIndexName.contains("IR Closeness"))
        userChoice=3;
    else if ( prominenceIndexName.contains("Betweenness Centr"))
        userChoice=4;
    else if (prominenceIndexName.contains("Stress Centr"))
        userChoice=5;
    else if (prominenceIndexName.contains("Eccentricity Centr"))
        userChoice=6;
    else if (prominenceIndexName.contains("Power Centr"))
        userChoice=7;
    else if (prominenceIndexName.contains("Information Centr"))
        userChoice=8;
    else if (prominenceIndexName.contains("Eigenvector Centr"))
        userChoice=9;
    else if (prominenceIndexName.contains("Degree Prestige"))
        userChoice=10;
    else if (prominenceIndexName.contains("PageRank Prestige"))
        userChoice=11;
    else if (prominenceIndexName.contains("Proximity Prestige"))
        userChoice=12;

    qDebug() << "MainWindow::slotLayoutNodeSizeByProminenceIndex() "
             << "prominenceIndexName " << prominenceIndexName
                << " userChoice " << userChoice;

    toolBoxLayoutByIndexSelect->setCurrentIndex(userChoice+1);
    toolBoxLayoutByIndexTypeSelect->setCurrentIndex(2);

    //check if CC was selected and the graph is disconnected.
    bool dropIsolates=false;
    if (userChoice == 2 ) {
        int connectedness=activeGraph.graphConnectedness();
        switch ( connectedness ) {
        case 1:
            break;
        case 2:
            break;
        case -1:
            if (! editFilterNodesIsolatesAct->isChecked() ) {
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
            }
            break;

        case -3:
            if (! editFilterNodesIsolatesAct->isChecked() ) {
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
            }
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
                   tr("Please note that this function is <b>SLOW</b> on large "
                      "networks (n>200), since it will calculate  a (n x n) matrix A with: <br>"
                      "Aii=1+weighted_degree_ni <br>"
                      "Aij=1 if (i,j)=0 <br>"
                      "Aij=1-wij if (i,j)=wij <br>"
                      "Next, it will compute the inverse matrix C of A. "
                      "The computation of the inverse matrix is a CPU intensive function "
                      "although it uses LU decomposition. <br>"
                      "How slow is this? For instance, to compute IC scores of 600 nodes "
                      "on a modern i7 4790K CPU you will need to wait for 2 minutes at least. <br>"
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

    graphicsWidget->clearGuides();

    activeGraph.layoutByProminenceIndex(
                userChoice, 2,
                considerWeights, inverseWeights,
                editFilterNodesIsolatesAct->isChecked() || dropIsolates);


    statusMessage( tr("Bigger nodes have greater %1 score.").arg(prominenceIndexName ) );
}








/**
 * @brief
 * Checks sender text() to find out who QMenu item was pressed
 * and what prominence index was chosen
 * calls slotLayoutNodeColorByProminenceIndex(QString)
  */
void MainWindow::slotLayoutNodeColorByProminenceIndex(){
    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    QAction *menuitem=(QAction *) sender();
    QString menuItemText = menuitem->text();
    qDebug() << "MainWindow::slotLayoutNodeColorByProminenceIndex() - " <<
                "SENDER MENU IS " << menuItemText;

    slotLayoutNodeColorByProminenceIndex(menuItemText);

}


/**
 * @brief Calls Graph::layoutVertexColorByProminenceIndex to apply a layout model
 * where the color of each node follows its prominence score
 * RED=rgb(255,0,0) most prominent
 * BLUE=rgb(0,0,255) least prominent
 * Called when selectbox changes in the toolbox
 */
void MainWindow::slotLayoutNodeColorByProminenceIndex(QString choice=""){
        qDebug() << "MainWindow::slotLayoutNodeColorByProminenceIndex() ";
    if ( !activeNodes() )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    int userChoice = 0;
    QString prominenceIndexName = choice;

    if ( prominenceIndexName.contains("Degree Centr") )
        userChoice=1;
    else if ( prominenceIndexName.contains("Closeness Centr") &&
              !prominenceIndexName.contains("IR"))
        userChoice=2;
    else if ( prominenceIndexName.contains("Influence Range Closeness Centrality") ||
              prominenceIndexName.contains("IR Closeness"))
        userChoice=3;
    else if ( prominenceIndexName.contains("Betweenness Centr"))
        userChoice=4;
    else if (prominenceIndexName.contains("Stress Centr"))
        userChoice=5;
    else if (prominenceIndexName.contains("Eccentricity Centr"))
        userChoice=6;
    else if (prominenceIndexName.contains("Power Centr"))
        userChoice=7;
    else if (prominenceIndexName.contains("Information Centr"))
        userChoice=8;
    else if (prominenceIndexName.contains("Eigenvector Centr"))
        userChoice=9;
    else if (prominenceIndexName.contains("Degree Prestige"))
        userChoice=10;
    else if (prominenceIndexName.contains("PageRank Prestige"))
        userChoice=11;
    else if (prominenceIndexName.contains("Proximity Prestige"))
        userChoice=12;

    qDebug() << "MainWindow::slotLayoutNodeColorByProminenceIndex() "
             << "prominenceIndexName " << prominenceIndexName
                << " userChoice " << userChoice;

    toolBoxLayoutByIndexSelect->setCurrentIndex(userChoice+1);
    toolBoxLayoutByIndexTypeSelect->setCurrentIndex(3);

    //check if CC was selected and the graph is disconnected.
    bool dropIsolates=false;
    if (userChoice == 2 ) {
        int connectedness=activeGraph.graphConnectedness();
        switch ( connectedness ) {
        case 1:
            break;
        case 2:
            break;
        case -1:
            if (! editFilterNodesIsolatesAct->isChecked() ) {
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
            }
            break;

        case -3:
            if (! editFilterNodesIsolatesAct->isChecked() ) {
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
            }
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
                   tr("Please note that this function is <b>SLOW</b> on large "
                      "networks (n>200), since it will calculate  a (n x n) matrix A with: <br>"
                      "Aii=1+weighted_degree_ni <br>"
                      "Aij=1 if (i,j)=0 <br>"
                      "Aij=1-wij if (i,j)=wij <br>"
                      "Next, it will compute the inverse matrix C of A. "
                      "The computation of the inverse matrix is a CPU intensive function "
                      "although it uses LU decomposition. <br>"
                      "How slow is this? For instance, to compute IC scores of 600 nodes "
                      "on a modern i7 4790K CPU you will need to wait for 2 minutes at least. <br>"
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

    graphicsWidget->clearGuides();


    activeGraph.layoutByProminenceIndex(
                userChoice, 3,
                considerWeights, inverseWeights,
                editFilterNodesIsolatesAct->isChecked() || dropIsolates);

    statusMessage( tr("Nodes with red color have greater %1 score.").arg(prominenceIndexName));

}






/**
 * @brief
 * Enables/disables layout guides
 * Called from
 * @param state
 */
void MainWindow::slotLayoutGuides(const bool &toggle){
    qDebug()<< "MW:slotLayoutGuides()";
    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    if (toggle){
        layoutGuidesAct->setChecked(true);
        qDebug()<< "MW:slotLayoutGuides() - will be displayed";
        statusMessage( tr("Layout Guides will be displayed") );
    }
    else {
        layoutGuidesAct->setChecked(false);
        qDebug()<< "MW:slotLayoutGuides() - will NOT be displayed";
        graphicsWidget->clearGuides();
        statusMessage( tr("Layout Guides will not be displayed") );
    }
}



/**
*	Returns the amount of enabled/active edges on the scene.
*/
int MainWindow::activeEdges(){
    qDebug () << "MW::activeEdges()";
    return activeGraph.edgesEnabled();
}





/**
*	Returns the number of active nodes on the scene.
*/
int MainWindow::activeNodes(){ 
    return activeGraph.vertices();
}






/**
*	Displays the arc and dyad reciprocity of the network
*/
void MainWindow::slotAnalyzeReciprocity(){

    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-reciprocity-"+dateTime+".html";

    askAboutWeights();

    activeGraph.writeReciprocity(fn, considerWeights);

    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage(tr("Reciprocity report saved as: ") + QDir::toNativeSeparators(fn) );

}



/**
*	Displays a box informing the user about the symmetry or not of the adjacency matrix
*/

void MainWindow::slotAnalyzeSymmetryCheck(){
    if ( !activeNodes() )   {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    if (activeGraph.graphSymmetric())
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



/**
 * @brief Writes the adjacency matrix inverse
 */
void MainWindow::slotAnalyzeMatrixAdjacencyInverse(){
    if ( !activeNodes() ) {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-matrix-adjacency-inverse-"+dateTime+".html";

    statusMessage(tr ("Inverting adjacency matrix.") );

    //activeGraph.writeMatrixAdjacencyInvert(fn, QString("lu")) ;
    activeGraph.writeMatrix(fn,MATRIX_ADJACENCY_INVERSE) ;

    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage(tr("Inverse matrix saved as: ")+QDir::toNativeSeparators(fn));
}






/**
 * @brief Writes the transpose adjacency matrix
 */
void MainWindow::slotAnalyzeMatrixAdjacencyTranspose(){
    if ( !activeNodes() ) {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-matrix-adjacency-transpose-"+dateTime+".html";

    statusMessage( tr ("Transposing adjacency matrix.") );

    activeGraph.writeMatrix(fn,MATRIX_ADJACENCY_TRANSPOSE) ;

    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage(tr("Transpose adjacency matrix saved as: ")+QDir::toNativeSeparators(fn));
}




/**
 * @brief Writes the cocitation matrix
 */
void MainWindow::slotAnalyzeMatrixAdjacencyCocitation(){
    if ( !activeNodes() ) {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-matrix-cocitation-"+dateTime+".html";

    statusMessage( tr ("Computing Cocitation matrix.") );

    activeGraph.writeMatrix(fn,MATRIX_COCITATION) ;

    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage(tr("Cocitation matrix saved as: ")+QDir::toNativeSeparators(fn));
}




/**
 * @brief Writes the degree matrix of the graph
 */
void MainWindow::slotAnalyzeMatrixDegree(){
    if ( !activeNodes() ) {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-matrix-degree-"+dateTime+".html";

    statusMessage(tr ("Computing Degree matrix.") );

    //activeGraph.writeMatrixDegreeText(fn) ;
    activeGraph.writeMatrix(fn, MATRIX_DEGREE) ;

    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage(tr("Degree matrix saved as: ")+QDir::toNativeSeparators(fn));
}




/**
 * @brief Writes the Laplacian matrix of the graph
 */
void MainWindow::slotAnalyzeMatrixLaplacian(){
    if ( !activeNodes() ) {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    qDebug() << "MW:slotAnalyzeMatrixLaplacian() - calling Graph::writeMatrix";

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-matrix-laplacian-"+dateTime+".html";

    statusMessage(tr ("Computing Laplacian matrix") );

    activeGraph.writeMatrix(fn, MATRIX_LAPLACIAN) ;

    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage(tr("Laplacian matrix saved as: ")+QDir::toNativeSeparators(fn));
}



/**
 * @brief If the network has weighted / valued edges, it asks the user
 * if the app should consider weights or not.
 */
void MainWindow::askAboutWeights(){

    qDebug() << "MW::askAboutWeights() - checking if graph weighted.";

    if (!activeGraph.graphWeighted()  ){
        considerWeights=false;
        return;
    }
    if (askedAboutWeights) {
        return;
    }

    if ( ! considerEdgeWeightsAct->isChecked() && !considerWeights){
        switch(
               slotHelpMessageToUser(USER_MSG_QUESTION, tr("Network is edge-weighted. Consider edge weights?"),
                                     tr("Edge-weighted network. Consider edge weights?"),
                                     tr("The edges in this network have weights (non-unit values). "
                                        "Take these edge weights into account to compute distances?"),
                                     QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes)

               )
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
        switch(

               slotHelpMessageToUser(
                   USER_MSG_QUESTION, tr("Inverse edge weights during calculations? "),
                   tr("Inverse edge weights during calculations? "),
                   tr("If the edge weights denote cost or real distances (i.e. miles between cities), "
                      "press No, since the distance between two nodes should be the quickest "
                      "or cheaper one. \n\n"
                      "If the weights denote value or strength (i.e. votes or interaction), "
                      "press Yes to inverse the weights, since the distance between two "
                      "nodes should be the most valuable one."),
                   QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes)

               )
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
 * @brief Displays the graph distance (geodesic distance) between two user-specified nodes
    This is the length of the shortest path between them.

 */
void MainWindow::slotAnalyzeDistance(){
    if ( !activeNodes() || !activeEdges()  )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    bool ok=false;
    long int  min=1, max=1, i=-1, j=-1;
    min=activeGraph.vertexNumberMin();
    max=activeGraph.vertexNumberMax();
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

    if (activeGraph.graphSymmetric() && i>j) {
        qSwap(i,j);
    }


    askAboutWeights();


     int distanceGeodesic = activeGraph.graphDistanceGeodesic(i,j,
                                         considerWeights,
                                         inverseWeights);

    if ( distanceGeodesic > 0 && distanceGeodesic < RAND_MAX)
        QMessageBox::information(this, tr("Geodesic Distance"),
                                 tr("The length of the shortest path between actors (")
                                 +QString::number(i)+", "+QString::number(j)
                                 +") = "+QString::number(distanceGeodesic)
                                 +tr("\nThe nodes are connected."),"OK",0);
    else
        QMessageBox::information(this, tr("Geodesic Distance"), tr("Network distance (")
                                 +QString::number(i)+", "+QString::number(j)
                                 +") = "+ QString("\xE2\x88\x9E")
                                 +tr("\nThe nodes are not connected."),"OK",0);
}




/**
 * @brief Invokes calculation of the matrix of geodesic distances for the loaded network, then displays it.
 */
void MainWindow::slotAnalyzeMatrixDistances(){
    qDebug() << "MW::slotAnalyzeMatrixDistances()";
    if ( !activeNodes()  )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-matrix-geodesic-distances-"+dateTime+".html";

    askAboutWeights();

    statusMessage( tr("Computing geodesic distances. Please wait...") );


    activeGraph.writeMatrix(fn,MATRIX_DISTANCES,
                            considerWeights, inverseWeights,
                            editFilterNodesIsolatesAct->isChecked());

    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage(tr("Geodesic Distances matrix saved as: ")+QDir::toNativeSeparators(fn));
}




/**
 * @brief Invokes calculation of the geodedics matrix (the number of shortest paths
 * between each pair of nodes in the loaded network), then displays it.
 */
void MainWindow::slotAnalyzeMatrixGeodesics(){
    qDebug("MW: slotViewNumberOfGeodesics()");
    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-matrix-geodesics-"+dateTime+".html";

    askAboutWeights();

    statusMessage(  tr("Computing geodesics (number of shortest paths). Please wait...") );

    activeGraph.writeMatrix(fn,MATRIX_GEODESICS,
                                    considerWeights, inverseWeights,
                                    editFilterNodesIsolatesAct->isChecked());

    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage(tr("Geodesics Matrix saved as: ") + QDir::toNativeSeparators(fn));
}



/**  Displays the network diameter (largest geodesic) */
void MainWindow::slotAnalyzeDiameter() {
    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    askAboutWeights();

    statusMessage(  QString(tr("Computing Graph Diameter. Please wait...")) );

    int netDiameter=activeGraph.graphDiameter(considerWeights, inverseWeights);

    if ( activeGraph.graphWeighted() && considerWeights )
        QMessageBox::information(this, "Diameter",
                                 tr("Diameter =  ")
                                 + QString::number(netDiameter) +
                                 tr("\n\nSince this is a weighted network \n"
                                 "the diameter can be more than N"),
                                 "OK",0);
    else if ( activeGraph.graphWeighted() && !considerWeights )
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
    statusMessage( tr("Graph Diameter computed. Ready.") );
}





/**  Displays the  average shortest path length (average graph distance) */
void MainWindow::slotAnalyzeDistanceAverage() {
    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    askAboutWeights();

    statusMessage(  tr("Computing Average Graph Distance. Please wait...") );

    float averGraphDistance=activeGraph.graphDistanceGeodesicAverage(
                considerWeights, inverseWeights,
                editFilterNodesIsolatesAct->isChecked() );

    QMessageBox::information(this,
                             "Average Graph Distance",
                             "The average shortest path length is  = " +
                             QString::number(averGraphDistance), "OK",0);

    statusMessage( tr("Average geodesic distance computed. Ready.") );

}


/**
*	Writes Eccentricity indices into a file, then displays it.
*/
void MainWindow::slotAnalyzeEccentricity(){
    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-eccentricity-"+dateTime+".html";

    askAboutWeights();

    activeGraph.writeEccentricity(
                fn, considerWeights, inverseWeights,
                editFilterNodesIsolatesAct->isChecked());

    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage(tr("Eccentricities saved as: ") + QDir::toNativeSeparators(fn) );
}




/**
 * @brief Displays the DialogDissimilarities dialog.
 */
void MainWindow::slotAnalyzeStrEquivalenceDissimilaritiesDialog() {
    qDebug()<< "MW::slotAnalyzeStrEquivalenceDissimilaritiesDialog()";

    m_dialogdissimilarities = new DialogDissimilarities(this);

    connect( m_dialogdissimilarities, &DialogDissimilarities::userChoices,
             this, &MainWindow::slotAnalyzeDissimilaritiesTieProfile );

    m_dialogdissimilarities->exec();

}





/**
 * @brief Invokes calculation of pair-wise tie profile dissimilarities of the
 * network, then displays it.
 * @param metric
 * @param varLocation
 * @param diagonal
 */
void MainWindow::slotAnalyzeDissimilaritiesTieProfile(const QString &metric,
                                                       const QString &varLocation,
                                                       const bool &diagonal){
    qDebug() << "MW::slotAnalyzeDissimilaritiesTieProfile()";
    if ( !activeNodes()    )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-matrix-tie-profile-dissimilarities-"+dateTime+".html";

    askAboutWeights();

    activeGraph.writeMatrixDissimilarities(fn, metric, varLocation,diagonal, considerWeights);

    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage(tr("Tie profile dissimilarities matrix saved as: ")+QDir::toNativeSeparators(fn));
}



/**
 * @brief Reports the network connectedness
 */
void MainWindow::slotAnalyzeConnectedness(){
    qDebug () << "MW::slotAnalyzeConnectedness()" ;
    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    int connectedness=activeGraph.graphConnectedness(true);

    qDebug () << "MW::slotAnalyzeConnectedness result " << connectedness;


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
*	Calls Graph:: writeWalksOfLengthMatrixPlainText() to calculate and print
*   the number of walks of a given length , between each pair of nodes.
*/
void MainWindow::slotAnalyzeWalksLength(){
    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

     bool ok=false;

    int length = QInputDialog::getInt(
                this, "Number of walks",
                tr("Select desired length of walk: (2 to %1)").arg(activeNodes()-1),
                2, 2, activeNodes()-1, 1, &ok );
    if (!ok) {
        statusMessage( "Cancelled." );
        return;
    }

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-matrix-walks-length-"+QString::number(length)+"-"+dateTime+".html";


    activeGraph.writeMatrixWalks(fn, length);


    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage(tr("Walks of length %1 matrix saved as: ").arg(length) + QDir::toNativeSeparators(fn) );
}



/**
 * @brief Calls Graph:: writeWalksTotalMatrixPlainText() to calculate and print
*  the total number of walks of any length , between each pair of nodes.
 */
void MainWindow::slotAnalyzeWalksTotal(){
    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    if (activeNodes() > 50) {
        switch( QMessageBox::critical(
                    this,
                    "Slow function warning",
                    tr("Please note that this function is VERY SLOW on large networks (n>50), "
                       "since it will calculate all powers of the sociomatrix up to n-1 "
                       "in order to find out all possible walks. \n\n"
                       "If you need to make a simple reachability test, "
                       "we advise to use the Reachability Matrix function instead. \n\n"
                       "Are you sure you want to continue?"),
                    QMessageBox::Ok|QMessageBox::Cancel,QMessageBox::Cancel) ) {
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

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-matrix-walks-total-"+dateTime+".html";

    statusMessage(  tr("Computing total walks matrix. Please wait...") );

    activeGraph.writeMatrixWalks(fn);

    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage("Total walks matrix saved as: " + QDir::toNativeSeparators(fn));

}



/**
*	Calls Graph:: writeReachabilityMatrixPlainText() to calculate and print
*   the Reachability Matrix of the network.
*/
void MainWindow::slotAnalyzeReachabilityMatrix(){
    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-matrix-reachability-"+dateTime+".html";

    statusMessage(  tr("Computing reachability matrix. Please wait...") );

    activeGraph.writeMatrix(fn, MATRIX_REACHABILITY );

    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage("Reachability matrix saved as: " + QDir::toNativeSeparators(fn) );
}




/**
 * @brief Displays the slotAnalyzeStrEquivalenceClusteringHierarchicalDialog dialog.
 */
void MainWindow::slotAnalyzeStrEquivalenceClusteringHierarchicalDialog() {
    qDebug()<< "MW::slotAnalyzeStrEquivalenceClusteringHierarchicalDialog()";

    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    QString preselectMatrix = "Adjacency";

    if (!activeGraph.graphWeighted()) {
        preselectMatrix = "Distances";
    }
    m_dialogClusteringHierarchical = new DialogClusteringHierarchical(this, preselectMatrix);

    connect( m_dialogClusteringHierarchical, &DialogClusteringHierarchical::userChoices,
             this, &MainWindow::slotAnalyzeClusteringHierarchical );

    m_dialogClusteringHierarchical->exec();

}



/**
 * @brief Called from DialogClusteringHierarchical with user choices. Calls
 * Graph::writeClusteringHierarchical() to compute and write HCA and displays the report.
 * @param matrix
 * @param similarityMeasure
 * @param linkageCriterion
 * @param diagonal
 */
void MainWindow::slotAnalyzeClusteringHierarchical(const QString &matrix,
                                                   const QString &metric,
                                                   const QString &method,
                                                   const bool &diagonal,
                                                   const bool &diagram){

    qDebug()<< "MW::slotAnalyzeClusteringHierarchical()";

    // No need to check as this method is called from a dialog which didi check for nodes
//    if ( !activeNodes()   )  {
//        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
//        return;
//    }

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-clustering-hierarchical-"+dateTime+".html";

    bool considerWeights=true;
    bool inverseWeights=false;
    bool dropIsolates=true;

    activeGraph.writeClusteringHierarchical(fn,
                                            matrix,
                                            metric,
                                            method,
                                            diagonal,
                                            diagram,
                                            considerWeights,
                                            inverseWeights,
                                            dropIsolates);


    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage("Hierarchical Cluster Analysis saved as: " + QDir::toNativeSeparators(fn));

}




/**
 * @brief Calls Graph:: writeCliqueCensus() to write the number of cliques (triangles)
*  of each vertex into a file, then displays it.
 */
void MainWindow::slotAnalyzeCommunitiesCliqueCensus(){

    if ( !activeNodes()  )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-clique-census-"+dateTime+".html";

    bool considerWeights=true;

    activeGraph.writeCliqueCensus(fn, considerWeights);

    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage("Clique Census saved as: " + QDir::toNativeSeparators(fn));
}






/**
 * @brief Calls Graph::writeClusteringCoefficient() to write Clustering Coefficients
 * into a file, and displays it.
 */
void MainWindow::slotAnalyzeClusteringCoefficient (){
    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-clustering-coefficient-"+dateTime+".html";

    bool considerWeights=true;

    activeGraph.writeClusteringCoefficient(fn, considerWeights);

    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage("Clustering Coefficients saved as: " + QDir::toNativeSeparators(fn));
}


/**
 * @brief Displays the DialogSimilarityMatches dialog.
 */
void MainWindow::slotAnalyzeStrEquivalenceSimilarityMeasureDialog() {
    qDebug()<< "MW::slotAnalyzeStrEquivalenceSimilarityMeasureDialog()";

    if ( !activeNodes()  )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    m_dialogSimilarityMatches = new DialogSimilarityMatches(this);

    connect( m_dialogSimilarityMatches, &DialogSimilarityMatches::userChoices,
             this, &MainWindow::slotAnalyzeSimilarityMatching );

    m_dialogSimilarityMatches->exec();

}




/**
 * @brief Calls Graph::writeMatrixSimilarityMatching() to write Exact Matches
 * similarity matrix into a file, and displays it.
 *
 */
void MainWindow::slotAnalyzeSimilarityMatching(const QString &matrix,
                                       const QString &varLocation,
                                       const QString &measure,
                                       const bool &diagonal) {
    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-matrix-similarity-matches"+dateTime+".html";

    bool considerWeights=true;

    //activeGraph.writeMatrixSimilarityMatchingPlain( fn, measure, matrix, varLocation, diagonal,considerWeights);
    activeGraph.writeMatrixSimilarityMatching( fn,
                                               measure,
                                               matrix,
                                               varLocation,
                                               diagonal,
                                               considerWeights);

    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage("Similarity matrix saved as: " + QDir::toNativeSeparators(fn));
}



/**
 * @brief Calls the m_dialogSimilarityPearson to display the Pearson statistics dialog
 */
void MainWindow::slotAnalyzeStrEquivalencePearsonDialog(){
    qDebug()<< "MW::slotAnalyzeStrEquivalencePearsonDialog()";
    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    m_dialogSimilarityPearson = new DialogSimilarityPearson(this);

    connect( m_dialogSimilarityPearson, &DialogSimilarityPearson::userChoices,
             this, &MainWindow::slotAnalyzeSimilarityPearson );

    m_dialogSimilarityPearson->exec();
}



/**
 * @brief Calls Graph::writeMatrixSimilarityPearson() to write Pearson
 * Correlation Coefficients into a file, and displays it.
 *
 */
void MainWindow::slotAnalyzeSimilarityPearson(const QString &matrix,
                                       const QString &varLocation,
                                       const bool &diagonal) {
    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-matrix-similarity-pearson-"+dateTime+".html";

    bool considerWeights=true;

    activeGraph.writeMatrixSimilarityPearson( fn, considerWeights, matrix, varLocation,diagonal);

    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage("Pearson correlation coefficients matrix saved as: " + QDir::toNativeSeparators(fn));
}


/**
*	Calls Graph to conduct and write a triad census into a file, then displays it.
*/
void MainWindow::slotAnalyzeCommunitiesTriadCensus() {

    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-triad-census-"+dateTime+".html";

    bool considerWeights=true;

    activeGraph.writeTriadCensus(fn, considerWeights);

    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage("Triad Census saved as: " + QDir::toNativeSeparators(fn));
}


/**
*	Writes Out-Degree Centralities into a file, then displays it.
*/
void MainWindow::slotAnalyzeCentralityDegree(){
    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    bool considerWeights=false;
    if ( activeGraph.graphWeighted()) {

        switch( slotHelpMessageToUser(
                    USER_MSG_QUESTION,
                    tr("Consider weights?") ,
                    tr("Graph edges have weights. \n"
                       "Take weights into account (Default: No)?")
                    )
                )
        {
        case QMessageBox::Yes:
            considerWeights=true;
            break;
        case QMessageBox::No:
            considerWeights=false;
            break;
        default: // just for sanity
            considerWeights=false;
            return;
            break;
        }
    }

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-centrality-out-degree-"+dateTime+".html";


    activeGraph.writeCentralityDegree(fn, considerWeights,
                                      editFilterNodesIsolatesAct->isChecked() );

    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage(tr("Out-Degree Centralities saved as: ") + QDir::toNativeSeparators(fn));
}




/**
*	Writes Closeness Centralities into a file, then displays it.
*/
void MainWindow::slotAnalyzeCentralityCloseness(){
    qDebug() << "MW::slotAnalyzeCentralityCloseness()";
    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
    statusMessage(  tr("Please wait while computing Connectivity...")  );
    int connectedness=activeGraph.graphConnectedness();
    QApplication::restoreOverrideCursor();

    bool dropIsolates=false;
    switch ( connectedness ) {
    case 1:
        break;
    case 2:
        break;
    case -1:
        if (! editFilterNodesIsolatesAct->isChecked() ) {
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
        }
        break;

    case -3:
        if (! editFilterNodesIsolatesAct->isChecked() ) {
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
        }
        break;
    default:
        QMessageBox::critical(this,
                              "Centrality Closeness",
                              tr(
                                  "Disconnected graph/digraph!\n"
                                  "Since this network is disconnected, "
                                  "the ordinary Closeness Centrality "
                                  "index is undefined, because d(u,v) will be "
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

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-centrality-closeness-"+dateTime+".html";


    activeGraph.writeCentralityCloseness(
                fn, considerWeights, inverseWeights,
                editFilterNodesIsolatesAct->isChecked() || dropIsolates);


    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage(tr("Closeness Centralities  saved as: ") + QDir::toNativeSeparators(fn));
}




/**
 * @brief MainWindow::slotAnalyzeCentralityClosenessIR
*	Writes Centrality Closeness (based on Influence Range) indices into a file,
*   then displays it.
 */
void MainWindow::slotAnalyzeCentralityClosenessIR(){
    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-centrality-closeness-influence-range-"+dateTime+".html";

    askAboutWeights();

    activeGraph.writeCentralityClosenessInfluenceRange(
                fn, considerWeights,inverseWeights,
                editFilterNodesIsolatesAct->isChecked());

    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage(tr("Influence Range Closeness Centralities saved as: ")+QDir::toNativeSeparators(fn));
}




/**
*	Writes Betweenness Centralities into a file, then displays it.
*/
void MainWindow::slotAnalyzeCentralityBetweenness(){
    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-centrality-betweenness-"+dateTime+".html";

    askAboutWeights();

    activeGraph.writeCentralityBetweenness(
                fn, considerWeights, inverseWeights,
                editFilterNodesIsolatesAct->isChecked());


    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage(tr("Betweenness Centralities saved as: ")+QDir::toNativeSeparators(fn));
}





/**
*	Writes Degree Prestige indices (In-Degree Centralities) into a file, then displays it.
*/
void MainWindow::slotAnalyzePrestigeDegree(){
    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    if (activeGraph.graphSymmetric()) {
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
    if ( activeGraph.graphWeighted()) {
        switch( QMessageBox::information( this, "Degree Prestige (In-Degree)",
                                          tr("Graph edges have weights. \n"
                                             "Take weights into account (Default: No)?"),
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

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-prestige-degree-"+dateTime+".html";

    activeGraph.writePrestigeDegree(fn, considerWeights,
                                    editFilterNodesIsolatesAct->isChecked() );

    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage(tr("Degree Prestige (in-degree) indices saved as: ") + QDir::toNativeSeparators(fn));
}



/**
*	Writes PageRank Prestige indices into a file, then displays it.
*/
void MainWindow::slotAnalyzePrestigePageRank(){
    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-prestige-pagerank-"+dateTime+".html";

    askAboutWeights();


    activeGraph.writePrestigePageRank(fn, editFilterNodesIsolatesAct->isChecked());


    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage(tr("PageRank Prestige indices saved as: ")+ QDir::toNativeSeparators(fn));
}



/**
 * @brief MainWindow::slotAnalyzePrestigeProximity
 * Writes Proximity Prestige indices into a file, then displays them.
 */
void MainWindow::slotAnalyzePrestigeProximity(){
    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-prestige-proximity-"+dateTime+".html";

    askAboutWeights();

    activeGraph.writePrestigeProximity(fn, true, false ,
                                       editFilterNodesIsolatesAct->isChecked());

    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage(tr("Proximity Prestige indices saved as: ")+ QDir::toNativeSeparators(fn));
}




/**
 * @brief MainWindow::slotAnalyzeCentralityInformation
 * Writes Informational Centralities into a file, then displays it.
 */
void MainWindow::slotAnalyzeCentralityInformation(){

    qDebug() << "MW::slotAnalyzeCentralityInformation()";

    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    if (activeNodes() > 200) {
        switch(
               QMessageBox::critical(
                   this, "Slow function warning",
                   tr("Please note that this function is <b>SLOW</b> on large "
                      "networks (n>200), since it will calculate  a (n x n) matrix A with: <br>"
                      "Aii=1+weighted_degree_ni <br>"
                      "Aij=1 if (i,j)=0 <br>"
                      "Aij=1-wij if (i,j)=wij <br>"
                      "Next, it will compute the inverse matrix C of A. "
                      "The computation of the inverse matrix is a CPU intensive function "
                      "although it uses LU decomposition. <br>"
                      "How slow is this? For instance, to compute IC scores of 600 nodes "
                      "on a modern i7 4790K CPU you will need to wait for 2 minutes at least. <br>"
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

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-centrality-information-"+dateTime+".html";

    askAboutWeights();

    activeGraph.writeCentralityInformation(fn,considerWeights, inverseWeights);

    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage(tr("Information Centralities saved as: ")+ QDir::toNativeSeparators(fn));
}






/**
 * @brief Writes Eigenvector Centralities into a file, then displays it.
 */
void MainWindow::slotAnalyzeCentralityEigenvector(){
    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-centrality-eigenvector-"+dateTime+".html";

    askAboutWeights();

    bool dropIsolates = false;

    activeGraph.writeCentralityEigenvector(fn,considerWeights, inverseWeights, dropIsolates);

    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage(tr("Eigenvector Centralities saved as: ")+ QDir::toNativeSeparators(fn));
}




/**
 * @brief MainWindow::slotAnalyzeCentralityStress
 * Writes Stress Centralities into a file, then displays it.
 */
void MainWindow::slotAnalyzeCentralityStress(){
    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-centrality-stress-"+dateTime+".html";

    askAboutWeights();


    activeGraph.writeCentralityStress(
                fn, considerWeights, inverseWeights,
                editFilterNodesIsolatesAct->isChecked());


    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage(tr("Stress Centralities saved as: ")+ QDir::toNativeSeparators(fn));
}





/**
 * @brief MainWindow::slotAnalyzeCentralityPower
 * Writes Gil-Schmidt Power Centralities into a file, then displays it.
 */
void MainWindow::slotAnalyzeCentralityPower(){
    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-centrality-power-Gil-Schmidt-"+dateTime+".html";

    askAboutWeights();

    activeGraph.writeCentralityPower(
                fn, considerWeights, inverseWeights,
                editFilterNodesIsolatesAct->isChecked());


    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage(tr("Gil-Schmidt Power Centralities saved as: ")+ QDir::toNativeSeparators(fn));
}




/**
 * @brief MainWindow::slotAnalyzeCentralityEccentricity
 * Writes Eccentricity Centralities into a file, then displays it.
 */
void MainWindow::slotAnalyzeCentralityEccentricity(){
    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-centrality-eccentricity-"+dateTime+".html";

    askAboutWeights();

    activeGraph.writeCentralityEccentricity(
                fn, considerWeights, inverseWeights,
                editFilterNodesIsolatesAct->isChecked());

    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage(tr("Eccentricity Centralities saved as: ")+ QDir::toNativeSeparators(fn));
}



/**
 * @brief Creates a Qt Progress Dialog
 * if max = 0, then max becomes equal to active vertices*
 * @param max
 * @param msg
 */
void MainWindow::slotProgressBoxCreate(const int &max, const QString &msg){
    qDebug() << "MW::slotProgressBoxCreate" ;

    if (  appSettings["showProgressBar"] == "true"  ){
        int duration = (max==0) ? activeNodes(): max;
        QProgressDialog *progressBox = new QProgressDialog(msg,
                                            "Cancel",
                                            0,
                                            duration,
                                             this);
        progressBox->setWindowModality(Qt::WindowModal);
        connect ( &activeGraph, &Graph::signalProgressBoxUpdate,
                  progressBox, &QProgressDialog::setValue );

        progressBox->setMinimumDuration(0);
        progressBox->setAutoClose(true);
        progressBox->setAutoReset(true);
        progressDialogs.push(progressBox);
    }

    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

}


/**
 * @brief Destroys the first in queue Progress dialog
 */
void MainWindow::slotProgressBoxDestroy(const int &max){
    qDebug () << "MainWindow::slotProgressBoxDestroy";
    QApplication::restoreOverrideCursor();
    if (  appSettings["showProgressBar"] == "true" && max > -1 ) {
        if (! progressDialogs.isEmpty()) {
            QProgressDialog *progressBox = progressDialogs.pop();
            progressBox->reset();
            progressBox->deleteLater();
            delete progressBox;
        }
    }
}





/**
 * @brief MainWindow::slotOptionsNodeNumbersVisibility
 * Turns on/off displaying the numbers of nodes (outside ones)
 * @param toggle
 */
void MainWindow::slotOptionsNodeNumbersVisibility(bool toggle) {
    qDebug() << "MW::slotOptionsNodeNumbersVisibility()" << toggle;
    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
    statusMessage( tr("Toggle Nodes Numbers. Please wait...") );
    appSettings["initNodeNumbersVisibility"] = (toggle) ? "true":"false";
    graphicsWidget->setNodeNumberVisibility(toggle);
    optionsNodeNumbersVisibilityAct->setChecked ( toggle );
    if (!toggle) {
        statusMessage( tr("Node Numbers are invisible now. "
                          "Click the same option again to display them.") );
    }
    else{
        statusMessage( tr("Node Numbers are visible again...") );
    }
    QApplication::restoreOverrideCursor();
    return;
}




/**
 * @brief MainWindow::slotOptionsNodeNumbersInside
 * Turns on/off displaying the nodenumbers inside the nodes.
 * @param toggle
 */
void MainWindow::slotOptionsNodeNumbersInside(bool toggle){
    qDebug() << "MW::slotOptionsNodeNumbersInside()" << toggle;

    statusMessage( tr("Toggle Numbers inside nodes. Please wait...") );
    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
    // if node numbers are hidden, show them first.
    if ( toggle && appSettings["initNodeNumbersVisibility"] != "true" )
        slotOptionsNodeNumbersVisibility(true);

    appSettings["initNodeNumbersInside"] = (toggle) ? "true":"false";
    activeGraph.vertexNumbersInsideNodesSet(toggle);
    graphicsWidget -> setNumbersInsideNodes(toggle);
    optionsNodeNumbersVisibilityAct->setChecked (toggle);
    if (toggle){
        statusMessage( tr("Numbers inside nodes...") );
    }
    else {
        statusMessage( tr("Numbers outside nodes...") );
    }
    QApplication::restoreOverrideCursor();
}





/**
 * @brief MainWindow::slotOptionsNodeLabelsVisibility
 * Turns on/off displaying labels
 * @param toggle
 */
void MainWindow::slotOptionsNodeLabelsVisibility(bool toggle){
    qDebug() << "MW::slotOptionsNodeLabelsVisibility()" << toggle;

    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
    statusMessage( tr("Toggle Nodes Labels. Please wait...") );
    appSettings["initNodeLabelsVisibility"] = (toggle) ? "true":"false";
    graphicsWidget->setNodeLabelsVisibility(toggle);
    activeGraph.vertexLabelsVisibilitySet(toggle);
    optionsNodeLabelsVisibilityAct->setChecked ( toggle );
    if (!toggle) {
        statusMessage( tr("Node Labels are invisible now. "
                          "Click the same option again to display them.") );
    }
    else{
        statusMessage( tr("Node Labels are visible again...") );
    }
    QApplication::restoreOverrideCursor();
}






/**
 * @brief MainWindow::slotOptionsEdgesVisibility
 * @param toggle
 */
void MainWindow::slotOptionsEdgesVisibility(bool toggle){
    if ( !activeEdges() ) {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
    statusMessage( tr("Toggle Edges Arrows. Please wait...") );
    appSettings["initEdgesVisibility"] = (toggle) ? "true": "false";
    graphicsWidget->setAllItemsVisibility(TypeEdge, toggle);
    if (!toggle) 	{
        statusMessage( tr("Edges are invisible now. Click again the same menu to display them.") );
    }
    else{
        statusMessage( tr("Edges visible again...") );
    }
    QApplication::restoreOverrideCursor();
}




/**
 * @brief MainWindow::slotOptionsEdgeArrowsVisibility
 * Turns on/off the arrows of edges
 * @param toggle
 */
void MainWindow::slotOptionsEdgeArrowsVisibility(bool toggle){
    if ( !activeNodes() ) {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    statusMessage( tr("Toggle Edges Arrows. Please wait...") );
    appSettings["initEdgeArrows"]= (toggle) ? "true":"false";

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
        appSettings["initEdgeArrows"]="true";
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
 * @brief MainWindow::slotOptionsEdgeWeightsDuringComputation
 * @param toggle
 */
void MainWindow::slotOptionsEdgeWeightsDuringComputation(bool toggle) {
   if (toggle) {
       considerWeights=true;
       askedAboutWeights=false;
       askAboutWeights(); // will only ask about inversion
   }
   else {
       considerWeights=false;
   }
   activeGraph.graphModifiedSet(GRAPH_CHANGED_EDGES);
}



/**
*  FIXME edges Bezier
*/
void MainWindow::slotOptionsEdgesBezier(bool toggle){
    if ( !activeNodes() ) {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
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
 * @brief MainWindow::slotOptionsEdgeThicknessPerWeight
 * @param toggle
 */
void MainWindow::slotOptionsEdgeThicknessPerWeight(bool toogle) {
    if (toogle) {

    }
    else {

    }
}





/**
 * @brief MainWindow::slotOptionsEdgeWeightNumbersVisibility
 * Turns on/off displaying edge weight numbers
 * @param toggle
 */
void MainWindow::slotOptionsEdgeWeightNumbersVisibility(bool toggle) {
    qDebug() << "MW::slotOptionsEdgeWeightNumbersVisibility - Toggling Edges Weights";
    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
    statusMessage( tr("Toggle Edges Weights. Please wait...") );
    appSettings["initEdgeWeightNumbersVisibility"] = (toggle) ? "true":"false";
    graphicsWidget->setEdgeWeightNumbersVisibility(toggle);
    activeGraph.edgeWeightNumbersVisibilitySet(toggle);
    optionsEdgeWeightNumbersAct->setChecked ( toggle );
    if (!toggle) {
        statusMessage( tr("Edge weights are invisible now. "
                          "Click the same option again to display them.") );
    }
    else{
        statusMessage( tr("Edge weights are visible again...") );
    }
    QApplication::restoreOverrideCursor();

}







/**
 * @brief MainWindow::slotOptionsEdgeLabelsVisibility
 * Turns on/off displaying edge labels
 * @param toggle
 */
void MainWindow::slotOptionsEdgeLabelsVisibility(bool toggle) {
    qDebug() << "MW::slotOptionsEdgeLabelsVisibility - Toggling Edges Weights";
    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
    statusMessage( tr("Toggle Edges Labels. Please wait...") );

    appSettings["initEdgeLabelsVisibility"] = (toggle) ? "true":"false";
    graphicsWidget->setEdgeLabelsVisibility(toggle);
    activeGraph.edgeLabelsVisibilitySet(toggle);
    optionsEdgeLabelsAct->setChecked ( toggle );
    if (!toggle) {
        statusMessage( tr("Edge labels are invisible now. "
                          "Click the same option again to display them.") );
    }
    else{
        statusMessage( tr("Edge labels are visible again...") );
    }
    QApplication::restoreOverrideCursor();

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
 * @brief Turns progress dialogs on or off
 * @param toggle
 *
 */
void MainWindow::slotOptionsProgressDialogVisibility(bool toggle) {
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
 * @brief MainWindow::slotOptionsBackgroundColor
 * Called from Options menu and Settings dialog
 * @param color QColor
 */
void MainWindow::slotOptionsBackgroundColor (QColor color){

    if (!color.isValid()) {
        color = QColorDialog::getColor( QColor ( appSettings["initBackgroundColor"] ),
                                        this,
                                               "Change the background color" );
    }
    if (color.isValid()) {
        appSettings["initBackgroundColor"] = color.name();
        QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
        graphicsWidget ->setBackgroundBrush(
                    QBrush(QColor (appSettings["initBackgroundColor"]))
                );
        QApplication::restoreOverrideCursor();
        statusMessage( tr("Ready. ")  );
    }
    else {
        // user pressed Cancel
        statusMessage( tr("Invalid color. ") );
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
    int randomTip=rand() % (tips.count()); //Pick a tip.
    QMessageBox::about( this, tr("Tip Of The Day"), tips[randomTip]);
}



/**
    Creates our tips.
*/
void MainWindow::slotHelpCreateTips(){
    tips+=tr("To create a new node: \n"
             "- double-click somewhere on the canvas \n"
             "- or press the keyboard shortcut CTRL+. (dot)\n"
             "- or press the Add Node button on the left panel");
    tips+=tr("SocNetV supports working with either undirected or directed data. "
             "When you start SocNetV for the first time, the application uses "
             "the 'directed data' mode; every edge you create is directed. "
             "To enter the 'undirected data' mode, press CTRL+E+U or enable the "
             "menu option Edit -> Edges -> Undirected Edges ");
    tips+=tr("If your screen is small, and the canvas appears even smaller "
             "hide the Control and/or Statistics panel. Then the canvas "
             "will expand to the whole application window. "
             "Open the Settings/Preferences dialog -> Window options and "
             "disable the two panels.");
    tips+=tr("A scale-free network is a network whose degree distribution follows a power law. "
             "SocNetV generates random scale-free networks according to the "
             "Barabási–Albert (BA) model using a preferential attachment mechanism.");
    tips+=tr("To delete a node permanently: \n"
             "- right-click on it and select Remove Node \n"
             "- or press CTRL+ALT+. and enter its number\n"
             "- or press the Remove Node button on the Control Panel");
    tips+=tr("To rotate the network: \n"
             " - drag the bottom slider to left or right \n"
             " - or click the buttons on the corners of the bottom slider\n"
             " - or press CTRL and the left or right arrow.");
    tips+=tr("To create a new edge between nodes A and B: \n"
             "- double-click on node A, then double-click on node B.\n"
             "- or middle-click on node A, and again on node B.\n"
             "- or right-click on the node, then select Add Edge from the popup.\n"
             "- or press the keyboard shortcut CTRL+/ \n"
             "- or press the Add Edge button on the Control Panel");
    tips+=tr("Add a label to an edge by right-clicking on it "
             "and selecting Change Label.");
    tips+=tr("You can change the background color of the canvas. "
             "Do it from the menu Options > View or "
             "permanently save this setting in Settings/Preferences.");
    tips+=tr("Default node colors, shapes and sizes can be changed. "
             "Open the Settings/Preferences dialog and use the "
             "options on the Node tab.");
    tips+=tr("The Statistics Panel shows network-level information (i.e. density) "
             "as well as info about any node you clicked on (inDegrees, "
             "outDegrees, clustering).");
    tips+=tr("You can move any node by left-clicking and dragging it with your mouse. "
             "If you want you can move multiple nodes at once. Left-click on empty space "
             "on the canvas and drag to create a rectangle selection around them. "
             "Then left-click on one of the selected nodes and drag it.");
    tips+=tr("To save the node positions in a network, you need to save your data "
             "in a format which supports node positions, suchs as GraphML or Pajek.");
    tips+=tr("Embed visualization models on the network from the options in "
             "the Layout menu or the select boxes on the left Control Panel. ");
    tips+=tr("To change the label of a node right-click on it, and click "
             "Selected Node Properties from the popup menu.");
    tips+=tr("All basic operations of SocNetV are available from the left Control panel "
             "or by right-clicking on a Node or an Edge or on canvas empty space.");
    tips+=tr("Node info (number, position, degree, etc) is displayed on the Status bar, "
             "when you left-click on it.");
    tips+=tr("Edge information is displayed on the Status bar, when you left-click on it.");
    tips+=tr("Save your work often, especially when working with large data sets. "
             "SocNetV alogorithms are faster when working with saved data. ");

    tips+=tr("The Closeness Centrality (CC) of a node v, is the inverse sum of "
       "the shortest distances between v and every other node. CC is "
       "interpreted as the ability to access information through the "
       "\'grapevine\' of network members. Nodes with high closeness "
       "centrality are those who can reach many other nodes in few steps. "
       "This index can be calculated in both graphs and digraphs. "
       "It can also be calculated in weighted graphs although the weight of "
       "each edge (v,u) in E is always considered to be 1. ");

    tips+=tr("The Information Centrality (IC) index counts all paths between "
       "nodes weighted by strength of tie and distance. "
       "This centrality  measure developed by Stephenson and Zelen (1989) "
       "focuses on how information might flow through many different paths. "
       "This index should be calculated only for undirected graphs. "
       "Note: To compute this index, SocNetV drops all isolated nodes.");

}





/**
 * @brief MainWindow::slotHelp
 * Opens the system web browser to load the online Manual
 */
void MainWindow::slotHelp(){
    statusMessage( tr("Opening the SocNetV Manual in your default web browser....") );
    QDesktopServices::openUrl(QUrl("http://socnetv.org/docs/index.html"));
}




/**
 * @brief MainWindow::slotHelpCheckUpdates
 * Opens a web browser to SocNetV website.
 */
void MainWindow::slotHelpCheckUpdates() {
    statusMessage( tr("Opening SocNetV website in your default web browser....") );
    QDesktopServices::openUrl(QUrl("http://socnetv.org/downloads?app=" + VERSION));
}



/**
    Displays the following message!!
*/
void MainWindow::slotHelpAbout(){
    int randomCookie=rand()%fortuneCookie.count();
QString BUILD="Wed Jul  5 14:27:58 EEST 2017";
    QMessageBox::about(
                this, tr("About SocNetV"),
                        tr("<b>Soc</b>ial <b>Net</b>work <b>V</b>isualizer (SocNetV)") +
                        tr("<p><b>Version</b>: ") + VERSION + "</p>" +
                        tr("<p><b>Build</b>: ")  + BUILD + " </p>" +

                        tr("<p>Website: <a href=\"http://socnetv.org\">http://socnetv.org</a></p>")+

                        tr("<p>(C) 2005-2017 by Dimitris V. Kalamaras</p>")+
                        tr("<p><a href=\"http://socnetv.org/contact\">Have questions? Contact us!</a></p>")+

                        tr("<p><b>Fortune cookie: </b><br> \"")  + fortuneCookie[randomCookie]  + "\"" +

                        tr("<p><b>License:</b><p>") +

                        tr("<p>This program is free software; you can redistribute it "
                           "and/or modify it under the terms of the GNU General "
                           "Public License as published by the Free Software Foundation; "
                           "either version 3 of the License, or (at your option) "
                           "any later version.</p>") +

                        tr("<p>This program is distributed in the hope that it "
                           "will be useful, but WITHOUT ANY WARRANTY; "
                           "without even the implied warranty of MERCHANTABILITY "
                           "or FITNESS FOR A PARTICULAR PURPOSE. "
                           "See the GNU General Public License for more details.</p>") +

                        tr("<p>You should have received a copy of the GNU "
                           "General Public License along with this program; "
                           "If not, see http://www.gnu.org/licenses/</p>"));
}



/**
    Creates the fortune cookies displayed on the above message.
*/
void MainWindow::createFortuneCookies(){
    fortuneCookie+="sic itur ad astra / sic transit gloria mundi ? <br /> "
                   "--Unknown";
    fortuneCookie+="The truth is not my business. I am a statistician... I don’t like words like \"correct\" and \"truth\". "
                    "Statistics is about measuring against convention. <br /> "
           "Walter Radermacher, Eurostat director, interview to NY Times, 2012.";
    fortuneCookie+="Losers of yesterday, the winners of tomorrow... <br /> "
                   "--B.Brecht";
    fortuneCookie+="I've seen things you people wouldn't believe. Attack ships on fire off the shoulder of Orion. " 
                   "I watched C-beams glitter in the dark near the Tannhauser gate. "
                   "All those moments will be lost in time... like tears in rain... Time to die.<br />"
                   "Replicant Roy Batty, Blade Runner (1982)";
    fortuneCookie+="Patriotism is the virtue of the wicked... <br /> "
                   "--O. Wilde";
    fortuneCookie+="No tengo nunca mas, no tengo siempre. En la arena <br />"
            "la victoria dejo sus piers perdidos.<br />"
            "Soy un pobre hombre dispuesto a amar a sus semejantes.<br />"
            "No se quien eres. Te amo. No doy, no vendo espinas. <br /> "
                   "--Pablo Neruda"  ;
    fortuneCookie+="Man must not check reason by tradition, but contrawise, "
                   "must check tradition by reason.<br> --Leo Tolstoy";
    fortuneCookie+="Only after the last tree has been cut down, <br>"
                   "only after the last river has been poisoned,<br> "
                   "only after the last fish has been caught,<br>"
                   "only then will you realize that money cannot be eaten. <br> "
                   "--The Cree People";
    fortuneCookie+="Stat rosa pristina nomine, nomina nuda tenemus <br >"
                   " --Unknown";
    fortuneCookie+="Jupiter and Saturn, Oberon, Miranda <br />"
            "And Titania, Neptune, Titan. <br />"
            "Stars can frighten. <br /> Syd Barrett";

}




/**
    Displays a short message about the Qt Toolbox.
*/
void MainWindow::slotAboutQt(){
    QMessageBox::aboutQt(this, "About Qt - SocNetV");
}



