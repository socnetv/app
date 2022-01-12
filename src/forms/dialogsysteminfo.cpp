/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 3.1.0-dev
 Written in Qt
 
                         dialogsysteminfo.cpp  -  description
                             -------------------
    copyright            : (C) 2005-2022 by Dimitris B. Kalamaras
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

#include "dialogsysteminfo.h"

#include <QDebug>
#include <QTextEdit>
#include <QPushButton>
#include <QScreen>
#include <QSysInfo>
#include <QSslSocket>
#include <QProcess>

#ifndef QT_NO_OPENGL
#include <QOpenGLFunctions>
#endif

#ifndef QT_NO_OPENGL
static QString getGlString(QOpenGLFunctions *functions, GLenum name)
{
    if (const GLubyte *p = functions->glGetString(name))
        return QString::fromLatin1(reinterpret_cast<const char *>(p));
    return QString();
}
#endif


DialogSystemInfo::DialogSystemInfo (QWidget *parent) :
    QDialog (parent),
    ui(new Ui::DialogSystemInfo)
{

    ui->setupUi(this);

    (ui->buttonBox) -> button (QDialogButtonBox::Ok) -> setDefault(true);

    (ui->infoTextEdit)->setFocus();

//    this->setMinimumHeight(QApplication::primaryScreen()->availableSize().height()/2);

    QString information;
    information += "<b>QT BUILD</b><br><br>";
    information += "Architecture: <br>" + QSysInfo::buildAbi() + "<br>";
    information += "<br><br>";

    information += "<b>SOCNETV BUILD</b><br><br>";
    information += "DirPath: <br>" + QApplication::applicationDirPath() + "<br><br>";
    information += "SSL version (at built-time): <br>" +  QSslSocket::sslLibraryBuildVersionString() + "<br>";
    information += "<br><br>";

    information += "<b>YOUR SYSTEM</b><br><br>";
    information += "OS: <br>" + QSysInfo::prettyProductName() + "<br><br>";
    information += "Kernel: <br>" + QSysInfo::kernelType() + " " + QSysInfo::kernelVersion() + "<br><br>";
    information += "Architecture: <br>" + QSysInfo::currentCpuArchitecture() + "<br><br>";

    if ( QSslSocket::supportsSsl() ) {
        information += "SSL support: <br>yes <br><br>";
        information += "SSL version (run-time): <br>" + QSslSocket::sslLibraryVersionString() + "<br><br>";

        information += "About web crawler: You are good to go. But please note, you may experience warnings/problems if you have a version of OpenSSL that does not match the one used while building SocNetV ";
        information += "(" +  QSslSocket::sslLibraryBuildVersionString() + ")<br>";
    }
    else {
        information += "SSL support: <br>NO<br><br>";
        #if defined(Q_OS_WIN)
        information += "About web crawler: If you want to use the web crawler with https:// urls, please install the same (or the closest) version of OpenSSL that was used while building your SocNetV application ";
        information += "(" +  QSslSocket::sslLibraryBuildVersionString() + ")";
        information += " You may download Win32/Win64 OpenSSL installers from: https://slproweb.com/products/Win32OpenSSL.html <br>";
        #else
        information += "About web crawler: If you want to use the web crawler with https:// urls, please install the same (or the closest) version of OpenSSL that was used while building your SocNetV application ";
        information += "(" +  QSslSocket::sslLibraryBuildVersionString() + "). <br>";
        #endif
    }

    #ifndef QT_NO_OPENGL
    QOpenGLFunctions qglFunctions(QOpenGLContext::currentContext());
    information += "<br>OpenGL: <br>";
    information += "Vendor: " + getGlString(&qglFunctions, GL_VENDOR) + "<br>";
    information += "Version: " + getGlString(&qglFunctions, GL_VERSION) + "<br>";
    information += "Renderer/Card: " + getGlString(&qglFunctions, GL_RENDERER) +  "<br>";
    #else
    information += "<br>OpenGL: <br>";
    information += "NONE. Build without OpenGL support!";
    #endif



    information += "<br>Library Paths: <br>" ;
    foreach(QString libPath, QCoreApplication::libraryPaths() ) {
        information += libPath + "<br>";
    }

    information += "<br><br>";


    information += "<b>YOUR SCREEN</b><br><br>";
    information += "Geometry: <br>";
    information += QString::number(QApplication::primaryScreen()->geometry().x());
    information += " x ";
    information += QString::number(QApplication::primaryScreen()->geometry().y());
    information += "<br><br>";
    information += "Size: <br>";
    information += QString::number(QApplication::primaryScreen()->size().width());
    information += " x ";
    information += QString::number(QApplication::primaryScreen()->size().height());
    information += "<br><br>";
    information += "Available Size: <br>";
    information += QString::number(QApplication::primaryScreen()->availableSize().width());
    information += " x ";
    information += QString::number(QApplication::primaryScreen()->availableSize().height());
    information += "<br><br>";
    information += "Device Pixel Ratio (the scale factor applied by the OS/Windowing system): <br>";
    information += QString::number(QApplication::primaryScreen()->devicePixelRatio());
    information += "<br><br>";
    information += "Logical DPI (i.e. 144 on Windows default 150% mode): <br>";
    information += QString::number(QApplication::primaryScreen()->logicalDotsPerInch());


    ui->infoTextEdit->setText(information);


   // connect ( ui.buttonBox,SIGNAL(accepted()), this, SLOT(getUserChoices()) );

}
