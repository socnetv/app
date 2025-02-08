/**
 * @file dialognodeedit.h
 * @brief Declares the DialogNodeEdit class for editing node properties in the network graph visualization.
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

#ifndef DIALOGNODEEDIT_H
#define DIALOGNODEEDIT_H

#include <QDialog>
#include <QHash>

namespace Ui
{
    class DialogNodeEdit;
}

class DialogNodeEdit : public QDialog
{
    Q_OBJECT
public:
    explicit DialogNodeEdit(QWidget *parent = Q_NULLPTR,
                            const QStringList &nodeShapeList = QStringList(),
                            const QStringList &iconPathList = QStringList(),
                            const QString &label = QString(),
                            const int &size = 0,
                            const QColor &color = QColor(),
                            const QString &shape = QString(),
                            const QString &path = QString(),
                            const QHash<QString, QString> &customAttributes = QHash<QString, QString>());

    ~DialogNodeEdit();

    void setCustomAttributes(const QHash<QString, QString> &attributes);

private slots:
    void checkErrors();
    void getNodeShape(const int &nodeShapeIndex);
    void getNodeIconFile();
    void getUserChoices();
    void selectColor();
    void on_addPropertyButton_clicked();
signals:
    void userChoices(const QString &label,
                     const int &size,
                     const QColor &color,
                     const QString &shape,
                     const QString &iconPath = QString(),
                     const QHash<QString, QString> &customAttributes = QHash<QString, QString>());
    void nodeEditDialogError(QString);

private:
    QStringList m_shapeList;
    QStringList m_iconList;
    QString nodeLabel;
    int nodeSize;
    QColor nodeColor;
    QString nodeShape;
    QString iconPath;
    QString nodeValue;
    QPixmap pixmap;
    Ui::DialogNodeEdit *ui;
    QHash<QString, QString> m_customAttributes;
};

#endif
