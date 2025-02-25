/**
 * @file dialogexportpdf.h
 * @brief Declares the DialogExportPDF class for exporting network visualizations as PDF files in SocNetV.
 * @author Dimitris B. Kalamaras
 * @copyright
 *   Copyright (C) 2005-2024 by Dimitris B. Kalamaras.
 *   This file is part of SocNetV (Social Network Visualizer).
 * @license
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, version 3 or later.
 *   For more details, see <http://www.gnu.org/licenses/>.
 * @see https://socnetv.org
 */

#ifndef DIALOGEXPORTPDF_H
#define DIALOGEXPORTPDF_H

#include <QDialog>
#include <QPrinter>
#include <QPageLayout>


namespace Ui {
class DialogExportPDF;
}


class DialogExportPDF : public QDialog
{
    Q_OBJECT
public:
    explicit DialogExportPDF (QWidget *parent = Q_NULLPTR );
    ~DialogExportPDF();
public slots:
    void checkFilename(const QString &fileName = QString());
    void getFilename();
    void getPrinterMode(const QString &mode);
    void getUserChoices ();
signals:
    void userChoices( QString &filename,
                      const QPageLayout::Orientation &orientation,
                      const int &dpi,
                      const QPrinter::PrinterMode printerMode,
                      const QPageSize &pageSize = QPageSize(QPageSize::A4));
private:
    QString m_fileName;
    int m_dpi;
    QPageLayout::Orientation m_orientation;
    QPrinter::PrinterMode m_printerMode;
    Ui::DialogExportPDF *ui;


};

#endif // DIALOGEXPORTPDF_H
