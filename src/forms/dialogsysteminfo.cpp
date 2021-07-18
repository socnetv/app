/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 3.0-dev
 Written in Qt
 
                         dialogsysteminfo.cpp  -  description
                             -------------------
    copyright            : (C) 2005-2021 by Dimitris B. Kalamaras
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

#ifndef QT_NO_OPENGL
#include <QOpenGLFunctions>
#endif


static QString getGlString(QOpenGLFunctions *functions, GLenum name)
{
    if (const GLubyte *p = functions->glGetString(name))
        return QString::fromLatin1(reinterpret_cast<const char *>(p));
    return QString();
}



DialogSystemInfo::DialogSystemInfo (QWidget *parent) :
    QDialog (parent),
    ui(new Ui::DialogSystemInfo)
{

    ui->setupUi(this);

    (ui->buttonBox) -> button (QDialogButtonBox::Ok) -> setDefault(true);

    (ui->infoTextEdit)->setFocus();

    QString information;
    information += "QT BUILD\n\n";
    information += QSysInfo::buildAbi();
    information += "\n\n";

    information += "SOCNETV BUILD\n\n";
    information += "DirPath: " + QApplication::applicationDirPath();
    information += "\n\n";

    information += "YOUR SYSTEM\n\n";

    information += "OS: " + QSysInfo::prettyProductName() + " Kernel: " + QSysInfo::kernelType() + " " + QSysInfo::kernelVersion() + "\n";
    information += "Architecture: " + QSysInfo::currentCpuArchitecture();
    information += "\n\n";


    information += "YOUR SCREEN \n\n";
    information += "Geometry: \n";
    information += QString::number(QApplication::primaryScreen()->geometry().x());
    information += " x ";
    information += QString::number(QApplication::primaryScreen()->geometry().y());
    information += "\n\n";
    information += "Size: \n";
    information += QString::number(QApplication::primaryScreen()->size().width());
    information += " x ";
    information += QString::number(QApplication::primaryScreen()->size().height());
    information += "\n\n";
    information += "Available Size: \n";
    information += QString::number(QApplication::primaryScreen()->availableSize().width());
    information += " x ";
    information += QString::number(QApplication::primaryScreen()->availableSize().height());
    information += "\n\n";
    information += "Device Pixel Ratio (the scale factor applied by the OS/Windowing system): \n";
    information += QString::number(QApplication::primaryScreen()->devicePixelRatio());
    information += "\n\n";
    information += "Logical DPI (i.e. 144 on Windows default 150% mode): \n";
    information += QString::number(QApplication::primaryScreen()->logicalDotsPerInch());

//    const QString glInfo = getGlString(topLevelGlWidget.context()->functions(), GL_VENDOR)
//        + QLatin1Char('/') + getGlString(topLevelGlWidget.context()->functions(), GL_RENDERER);

//    const bool supportsThreading = !glInfo.contains(QLatin1String("nouveau"), Qt::CaseInsensitive)
//        && !glInfo.contains(QLatin1String("ANGLE"), Qt::CaseInsensitive)
//        && !glInfo.contains(QLatin1String("llvmpipe"), Qt::CaseInsensitive);



    ui->infoTextEdit->setText(information);


   // connect ( ui.buttonBox,SIGNAL(accepted()), this, SLOT(getUserChoices()) );

}
