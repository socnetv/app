/****************************************************************************
SocNetV: Social Networks Visualizer 
version: 1.6
Written in Qt

                          htmlviewer.cpp  -  description
                             -------------------
    copyright            : (C) 2005-2015 by Dimitris B. Kalamaras
    email                : dimitris.kalamaras@gmail.com

*****************************************************************************/

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

#include "htmlviewer.h"
#include <QtGui>
#include <QMenu>
#include <QMenuBar>
#include <QToolBar>
#include <QComboBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QWebView>

HTMLViewer::HTMLViewer( const QString& manPath,  QWidget* parent)
    :QMainWindow( parent ),
      pathCombo( 0 )
{
    view = new QWebView(this);
    view->load( QUrl::fromLocalFile(manPath) );
    this->setCentralWidget( view);

    QMenu* file = new QMenu( this );
    file = menuBar()->addMenu(tr("&File"));

    fileOpen = new QAction(QIcon(":/images/open.png"), tr("&Open"), this);
    fileOpen->setShortcut(tr("Ctrl+O"));
    fileOpen->setStatusTip(tr("Opens another helpfile"));
    connect(fileOpen, SIGNAL(triggered()), this, SLOT(openFile()));


    filePrint = new QAction(QIcon(":/images/print.png"), tr("&Print"), this);
    filePrint->setShortcut(tr("Ctrl+P"));
    filePrint->setStatusTip(tr("Prints out the actual network"));
    connect(filePrint, SIGNAL(triggered()), this, SLOT(print()));

    fileQuit = new QAction(QIcon(":/images/exit.png"), tr("E&xit"), this);
    fileQuit->setShortcut(tr("Ctrl+X"));
    fileQuit->setStatusTip(tr("Close Manual"));
    connect(fileQuit, SIGNAL(triggered()), this, SLOT(close()));

    QIcon icon_back( ":/images/back.png" );
    QIcon icon_forward( ":/images/forward.png" );
    QIcon icon_home( ":/images/home.png" );

    fileBack= new QAction(icon_back, tr("&Back"), this);
    fileBack->setShortcut(tr("Ctrl+B"));
    fileBack->setStatusTip(tr("&Backward") );
    connect(fileBack, SIGNAL(triggered()), view, SLOT(back()));

    fileForward= new QAction(icon_forward, tr("&Forward"), this);
    fileForward->setShortcut(tr("Ctrl+F"));
    fileForward->setStatusTip(tr("&Forward") );
    connect(fileForward, SIGNAL(triggered()), view, SLOT(forward()));

    fileHome= new QAction(icon_home, tr("&Home"), this);
    fileHome->setShortcut(tr("Ctrl+H"));
    fileHome->setStatusTip(tr("&Home") );
    connect(fileHome, SIGNAL(triggered()), view, SLOT(reload()));


    file->addAction( fileOpen);
//    file->addAction( filePrint);
    file->addSeparator();
    file->addAction( fileQuit);


    QMenu* go = new QMenu( this );
    go = menuBar()->addMenu(tr("&Go"));
    go->addAction( fileBack );
    go->addAction( fileForward );
    go->addAction( fileHome);


//     QMenu* help = new QMenu( this );
//     help= menuBar()->addMenu(tr("&Help"));
//     help->addAction( tr("&About"), this, SLOT( about() ) );
//     help->addAction( tr("About &Qt"), this, SLOT( aboutQt() ) );



    QToolBar* toolbar = new QToolBar( this );
    addToolBar( Qt::TopToolBarArea, toolbar);

    pathCombo = new QComboBox( toolbar );
    connect( pathCombo, SIGNAL( activated( const QString & ) ),
	     this, SLOT( pathSelected( const QString & ) ) );

    pathCombo->addItem(manPath);

    toolbar->addAction(fileBack);
    toolbar->addAction(fileForward);
    toolbar->addAction(fileHome);
    toolbar->addWidget( pathCombo );

    view->setFocus();

	this-> resize(700,650);
}




HTMLViewer::~HTMLViewer()
{
}


void HTMLViewer::openFile()
{
#ifndef QT_NO_FILEDIALOG

    QString fn = QFileDialog::getOpenFileName( this, QString::null, QString::null );
    if ( !fn.isEmpty() ) {
        QFile file(fn);

        if (!file.open(QIODevice::ReadOnly)) {
            QMessageBox::information(this, tr("Unable to open file"),
                                     file.errorString());
            return;
        }

//        QTextStream out(&file);
//        QString output = out.readAll();


        view->load( QUrl::fromLocalFile(fn) );
    }
#endif
}



void HTMLViewer::pathSelected( const QString &_path )
{
    	view->load(QUrl(_path));
}



