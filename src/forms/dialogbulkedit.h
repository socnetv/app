/**
 * @file dialogbulkedit.h
 * @brief Declares DialogBulkEdit for applying a single property value to multiple nodes or edges.
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

#ifndef DIALOGBULKEDIT_H
#define DIALOGBULKEDIT_H

#include <QDialog>
#include <QColor>
#include <QPixmap>
#include <QStringList>
#include <QVector>

namespace Ui { class DialogBulkEdit; }

class DialogBulkEdit : public QDialog
{
    Q_OBJECT

public:
    enum class Scope { Nodes, Edges };

    explicit DialogBulkEdit(Scope scope,
                            const QStringList &existingKeys,
                            const QStringList &nodeShapeList = QStringList(),
                            const QStringList &iconPathList  = QStringList(),
                            int targetCount                  = 0,
                            bool isFilterScope               = false,
                            QWidget *parent                  = nullptr);
    ~DialogBulkEdit();

signals:
    void userChoices(const QString &property, const QString &value);

private slots:
    void onPropertyChanged(int index);
    void onColorButtonClicked();
    void getUserChoices();

private:
    Ui::DialogBulkEdit *ui;

    QStringList  m_nodeShapeList;
    QStringList  m_iconPathList;
    QColor       m_color;
    QPixmap      m_pixmap;

    QStringList  m_propertyKeys;   // internal key per combo item (empty = separator)
    QVector<int> m_propertyPages;  // valueStack page index per combo item (-1 = separator)

    static constexpr int PAGE_TEXT   = 0;
    static constexpr int PAGE_INT    = 1;
    static constexpr int PAGE_DOUBLE = 2;
    static constexpr int PAGE_COLOR  = 3;
    static constexpr int PAGE_SHAPE  = 4;
};

#endif // DIALOGBULKEDIT_H
