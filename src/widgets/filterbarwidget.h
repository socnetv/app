/**
 * @file filterbarwidget.h
 * @brief Persistent filter bar showing active filter chips (#219).
 * @author Dimitris B. Kalamaras
 * @copyright
 *   Copyright (C) 2005-2026 by Dimitris B. Kalamaras.
 *   This file is part of SocNetV (Social Network Visualizer).
 * @license
 *   GNU GPL v3 or later. See <http://www.gnu.org/licenses/>.
 * @see https://socnetv.org
 */

#pragma once

#include <QWidget>
#include <QList>
#include "../graph/filters/filter_condition.h"

class QHBoxLayout;
class QPushButton;
class QFrame;

/**
 * @brief Thin strip between toolbar and canvas showing one chip per active filter.
 *
 * Hidden when no filters are active. Each chip carries a label and a × button.
 * Clicking × removes the chip and emits chipCloseRequested() so MainWindow can
 * pop the corresponding snapshot. "Clear all" emits clearAllRequested() and lets
 * MainWindow drain the stack before calling clearAllChips().
 *
 * Menu-driven restores (Restore All Nodes / Restore All Edges) must call
 * removeLatestChipForScope() / removeAllChipsForScope() so the bar stays in sync.
 */
class FilterBarWidget : public QWidget
{
    Q_OBJECT
public:
    explicit FilterBarWidget(QWidget *parent = nullptr);

    void addChip(const QString &label, FilterCondition::Scope scope);
    void removeLatestChipForScope(FilterCondition::Scope scope);
    void removeAllChipsForScope(FilterCondition::Scope scope);

public slots:
    void clearAllChips();

signals:
    /** Emitted after the chip has already been removed from the bar. */
    void chipCloseRequested(FilterCondition::Scope scope);
    /** Emitted when the user clicks "Clear all". */
    void clearAllRequested();

private:
    struct ChipData {
        FilterCondition::Scope scope;
        QFrame *frame = nullptr;
    };

    QHBoxLayout *m_chipsLayout;
    QPushButton *m_clearAllBtn;
    QList<ChipData> m_chips;

    void removeChip(QFrame *chip);
    void updateVisibility();
};
