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
class QToolButton;

/**
 * @brief Thin strip between toolbar and canvas showing one chip per active filter.
 *
 * Hidden when no filters are active. Each chip carries a label and a × button.
 * Clicking × emits chipCloseRequested(barIndex, scope) BEFORE any chip is removed.
 * The receiver (MainWindow) orchestrates graph state restoration, then calls
 * removeChipAt() or rebuilds the bar via clearAllChips() + addChip().
 * "Clear all" emits clearAllRequested(); MainWindow drains the stack, then calls
 * clearAllChips().
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
    void removeChipAt(int barIndex);
    FilterCondition::Scope chipScopeAt(int barIndex) const;

public slots:
    void clearAllChips();

signals:
    /**
     * Emitted when the user clicks × on a chip, BEFORE the chip is removed.
     * @p barIndex is the chip's position (0 = leftmost) at the time of the click.
     * The bar does NOT remove the chip automatically; the receiver must call
     * removeChipAt(), clearAllChips(), or rebuild the bar as appropriate.
     */
    void chipCloseRequested(int barIndex, FilterCondition::Scope scope);
    /** Emitted when the user clicks "Clear all". */
    void clearAllRequested();

private:
    struct ChipData {
        FilterCondition::Scope scope;
        QFrame *frame = nullptr;
        QToolButton *closeBtn = nullptr;
    };

    QHBoxLayout *m_chipsLayout;
    QPushButton *m_clearAllBtn;
    QList<ChipData> m_chips;

    void removeChip(QFrame *chip);
    void updateVisibility();
    void updateCloseButtons();
};
