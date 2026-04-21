/**
 * @file dialogedgeedit.h
 * @brief Declares the DialogEdgeEdit class for editing edge properties.
 * @author Dimitris B. Kalamaras
 * @copyright
 *   Copyright (C) 2005-2026 by Dimitris B. Kalamaras.
 *   This file is part of SocNetV (Social Network Visualizer).
 * @license
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, version 3 or later.
 *   For more details, see <http://www.gnu.org/licenses/>.
 * @see https://socnetv.org
 */

#ifndef DIALOGEDGEEDIT_H
#define DIALOGEDGEEDIT_H

#include <QDialog>
#include <QHash>
#include <QColor>
#include <QPixmap>

namespace Ui
{
    class DialogEdgeEdit;
}

class DialogEdgeEdit : public QDialog
{
    Q_OBJECT
public:
    explicit DialogEdgeEdit(QWidget *parent,
                            const int &v1,
                            const int &v2,
                            const QString &label,
                            const double &weight,
                            const QColor &color,
                            const QHash<QString,QString> &customAttributes = QHash<QString,QString>());
    ~DialogEdgeEdit();

private slots:
    void getUserChoices();
    void selectColor();
    void on_addPropertyBtn_clicked();
    void on_removePropertyBtn_clicked();

signals:
    void userChoices(const QString &label,
                     const double &weight,
                     const QColor &color,
                     const QHash<QString,QString> &customAttributes);

private:
    Ui::DialogEdgeEdit *ui;
    int m_v1, m_v2;
    QString m_label;
    double m_weight;
    QColor m_color;
    QHash<QString,QString> m_customAttributes;
    QPixmap m_pixmap;
};

#endif
