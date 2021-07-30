/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 3.0.1
 Written in Qt

                         dialogexportimage.h  -  description
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


#ifndef DIALOGEXPORTIMAGE_H
#define DIALOGEXPORTIMAGE_H

#include <QDialog>

namespace Ui {
class DialogExportImage;
}

class DialogExportImage : public QDialog
{
    Q_OBJECT

public:
    explicit DialogExportImage(QWidget *parent = Q_NULLPTR);
    ~DialogExportImage();
    void changeCompressionRange(const int &min, const int &max, const int &step);
    void changeQualityRange(const int &min, const int &max, const int &step);

public slots:
    void getFilename();
    void getFormat(const QString &format);
    void getUserChoices();

signals:
    void userChoices( const QString &filename,
                      const QByteArray &format,
                      const int &quality,
                      const int &compression
                      );

private:
    Ui::DialogExportImage *ui;
};

#endif // DIALOGEXPORTIMAGE_H
