/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 3.0-dev
 Written in Qt

                        main.cpp  -  description
                             -------------------
    copyright         : (C) 2005-2021 by Dimitris B. Kalamaras
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
********************************************************************************/

#include <QApplication>		//core Qt functionality
#include <QFile>
#include <QTranslator>		//for text translations
#include <QLocale>
#include <QSurfaceFormat>
#include <QCommandLineParser>
#include "mainwindow.h"		//main application window

using namespace std;

int main(int argc, char *argv[])
{
    Q_INIT_RESOURCE(src);

    //
    // Set the global default surface format to enable multisampling
    // used by default in QOpenGLContext, QWindow, QOpenGLWidget and similar classes.
    //
    QSurfaceFormat fmt;
    fmt.setSamples(4);
    QSurfaceFormat::setDefaultFormat(fmt);

    //
    // Create the application instance
    //
    QApplication app(argc, argv);

    //
    // Load our default stylesheet
    //
    QString sheetName = "default.qss";
    QFile file(":/qss/" + sheetName );

    if (!file.open(QFile::ReadOnly)) {
        qDebug () << "Could not open stylesheet file:" << file.fileName();
    }
    else {
        // Read stylesheet
        QString styleSheet = QString::fromLatin1(file.readAll());
        // Apply our default stylesheet to the app
        qApp->setStyleSheet(styleSheet);

    }
    file.close();


    //
    // Setup app translations
    //
    // Todo update/remove translations
    QTranslator tor( 0 );
    QLocale locale;

    // set the location where .qm files are in load() below as the last parameter instead of "."
    // for development, use "/" to use the english original as
    // .qm files are stored in the base project directory.

    tor.load( QString("socnetv.") + locale.name(), "." );
    app.installTranslator( &tor );

    //
    // Set application basic info
    //
    app.setOrganizationName("socnetv");
    app.setOrganizationDomain("socnetv.org");
    app.setApplicationDisplayName("SocNetV");   // Used in widgets

    app.setApplicationName("Social Network Visualizer");    // used by windowing system
    app.setApplicationVersion(VERSION);

    //
    // Setup the command line parser
    //
    QCommandLineParser parser;

    QString cmdDescr = "\nSocial Network Visualizer, version " + (VERSION) + "\n\n"
            "Copyright: Dimitris V. Kalamaras <dimitris.kalamaras@gmail.com>\n" +
            "License: GPL3";
    parser.setApplicationDescription(cmdDescr);

    parser.addHelpOption();
    parser.addVersionOption();

    parser.addPositionalArgument( "file",
                                  QCoreApplication::translate("main", "Network file to load on startup. You can load a network from a file using `socnetv file.net` where file.net/csv/dot/graphml must be of valid format. See README.")
                                  );

    // A boolean option for progress dialogs
    QCommandLineOption showProgressOption(QStringList() << "p" << "progress", QCoreApplication::translate("main", "Force showing progress dialogs/bars during computations."));
    parser.addOption(showProgressOption);

    // A boolean option for maximized display
    QCommandLineOption showMaximizedOption(QStringList() << "m" << "maximized", QCoreApplication::translate("main", "Show app maximized."));
    parser.addOption(showMaximizedOption);

    // A boolean option for full screen display
    QCommandLineOption showFullScreenOption(QStringList() << "f" << "fullscreen", QCoreApplication::translate("main", "Show in full screen mode."));
    parser.addOption(showFullScreenOption);

    // An option to enable debug messges with a verbosity value
    QCommandLineOption showDebugOption(QStringList() << "d" << "debug",
                                             QCoreApplication::translate("main", "Print debug messages to stdout/console. Available verbosity <level>s: 'none', 'min' or 'full'. Default: 'min'."),
                                             QCoreApplication::translate("main", "level"));
    parser.addOption(showDebugOption);

    // Process the actual command line arguments given by the user
    parser.process(app);

    // Read positional arguments
    const QStringList args = parser.positionalArguments();
    QString fileName;
    if ( !args.isEmpty() ) {
        fileName= args.at(0);
    }

    bool showProgress = parser.isSet(showProgressOption);
    bool showMaximized = parser.isSet(showMaximizedOption);
    bool showFullScreen= parser.isSet(showFullScreenOption);
    bool showDebug = parser.isSet(showDebugOption);
    int debugLevel = -1; // By default, we assume no debug option was passed
    if (showDebug) {
        if (parser.value(showDebugOption) == "full") {
            debugLevel = 2;
        }
        else if (parser.value(showDebugOption) == "min") {
            debugLevel = 1;
        }
        else {
            // Any other value/string, disables the debugging
            debugLevel = 0;
        }
    }

    //
    // Create our MainWindow and exec the app to enter the main event loop.
    //
    MainWindow *socnetv=new MainWindow(fileName, showProgress, showMaximized, showFullScreen, debugLevel);

    // Show the application
    socnetv->show();

    return app.exec();
}


