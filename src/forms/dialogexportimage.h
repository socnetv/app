/**
 * @file dialogexportimage.h
 * @brief Declares the DialogExportImage class for exporting network visualizations as images in SocNetV.
 * @author Dimitris B. Kalamaras
 * @copyright
 *   Copyright (C) 2005-2025 by Dimitris B. Kalamaras.
 *   This file is part of SocNetV (Social Network Visualizer).
 * @license
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, version 3 or later.
 *   For more details, see <http://www.gnu.org/licenses/>.
 * @see https://socnetv.org
 */

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
