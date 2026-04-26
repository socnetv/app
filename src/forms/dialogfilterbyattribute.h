/**
 * @file dialogfilterbyattribute.h
 * @brief Declares DialogFilterByAttribute for filtering nodes/edges by a custom attribute.
 * @author Dimitris B. Kalamaras
 * @copyright
 *   Copyright (C) 2005-2026 by Dimitris B. Kalamaras.
 *   This file is part of SocNetV (Social Network Visualizer).
 * @license
 *   GNU GPL v3 or later. See <http://www.gnu.org/licenses/>.
 * @see https://socnetv.org
 */

#ifndef DIALOGFILTERBYATTRIBUTE_H
#define DIALOGFILTERBYATTRIBUTE_H

#include <QDialog>
#include <QStringList>
#include "graph/filters/filter_condition.h"

namespace Ui { class DialogFilterByAttribute; }

class DialogFilterByAttribute : public QDialog
{
    Q_OBJECT
public:
    explicit DialogFilterByAttribute(const QStringList &nodeKeys,
                                     const QStringList &edgeKeys,
                                     QWidget *parent = nullptr);
    ~DialogFilterByAttribute();

private slots:
    void onScopeChanged();
    void getUserChoices();

signals:
    void userChoices(const FilterCondition &cond);

private:
    Ui::DialogFilterByAttribute *ui;
    QStringList m_nodeKeys;
    QStringList m_edgeKeys;

    void repopulateKeys(FilterCondition::Scope scope);
};

#endif // DIALOGFILTERBYATTRIBUTE_H
