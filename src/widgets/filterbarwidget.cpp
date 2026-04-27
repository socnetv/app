/**
 * @file filterbarwidget.cpp
 * @brief Implements FilterBarWidget — the persistent filter chip bar (#219).
 * @author Dimitris B. Kalamaras
 * @copyright
 *   Copyright (C) 2005-2026 by Dimitris B. Kalamaras.
 *   This file is part of SocNetV (Social Network Visualizer).
 * @license
 *   GNU GPL v3 or later. See <http://www.gnu.org/licenses/>.
 * @see https://socnetv.org
 */

#include "filterbarwidget.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QFrame>
#include <QLabel>
#include <QToolButton>
#include <QIcon>
#include <QSize>

FilterBarWidget::FilterBarWidget(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("FilterBarWidget");
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QHBoxLayout *outer = new QHBoxLayout(this);
    outer->setContentsMargins(6, 3, 6, 3);
    outer->setSpacing(4);

    m_chipsLayout = new QHBoxLayout;
    m_chipsLayout->setContentsMargins(0, 0, 0, 0);
    m_chipsLayout->setSpacing(4);

    outer->addLayout(m_chipsLayout);
    outer->addStretch(1);

    m_clearAllBtn = new QPushButton(tr("Clear all"), this);
    m_clearAllBtn->setObjectName("ClearAllButton");
    m_clearAllBtn->setFlat(true);
    m_clearAllBtn->setCursor(Qt::PointingHandCursor);
    connect(m_clearAllBtn, &QPushButton::clicked,
            this, &FilterBarWidget::clearAllRequested);
    outer->addWidget(m_clearAllBtn);

    hide();
}

void FilterBarWidget::addChip(const QString &label, FilterCondition::Scope scope)
{
    QFrame *chip = new QFrame(this);
    chip->setObjectName("FilterChip");
    chip->setFrameShape(QFrame::StyledPanel);

    QHBoxLayout *lay = new QHBoxLayout(chip);
    lay->setContentsMargins(8, 2, 2, 2);
    lay->setSpacing(2);

    QLabel *lbl = new QLabel(label, chip);
    lbl->setObjectName("ChipLabel");

    QToolButton *closeBtn = new QToolButton(chip);
    closeBtn->setObjectName("ChipCloseButton");
    closeBtn->setIcon(QIcon(":/images/close_24px.svg"));
    closeBtn->setIconSize(QSize(12, 12));
    closeBtn->setAutoRaise(true);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setToolTip(tr("Remove this filter"));

    lay->addWidget(lbl);
    lay->addWidget(closeBtn);

    m_chipsLayout->addWidget(chip);
    m_chips.append({scope, chip, closeBtn});

    connect(closeBtn, &QToolButton::clicked, this, [this, chip, scope]() {
        removeChip(chip);
        emit chipCloseRequested(scope);
    });

    updateVisibility();
}

void FilterBarWidget::removeChip(QFrame *chip)
{
    for (int i = 0; i < m_chips.size(); ++i) {
        if (m_chips[i].frame == chip) {
            m_chipsLayout->removeWidget(chip);
            chip->deleteLater();
            m_chips.removeAt(i);
            break;
        }
    }
    updateVisibility();
}

void FilterBarWidget::removeLatestChipForScope(FilterCondition::Scope scope)
{
    for (int i = m_chips.size() - 1; i >= 0; --i) {
        if (m_chips[i].scope == scope) {
            m_chipsLayout->removeWidget(m_chips[i].frame);
            m_chips[i].frame->deleteLater();
            m_chips.removeAt(i);
            break;
        }
    }
    updateVisibility();
}

void FilterBarWidget::removeAllChipsForScope(FilterCondition::Scope scope)
{
    for (int i = m_chips.size() - 1; i >= 0; --i) {
        if (m_chips[i].scope == scope) {
            m_chipsLayout->removeWidget(m_chips[i].frame);
            m_chips[i].frame->deleteLater();
            m_chips.removeAt(i);
        }
    }
    updateVisibility();
}

void FilterBarWidget::clearAllChips()
{
    for (auto &cd : m_chips) {
        m_chipsLayout->removeWidget(cd.frame);
        cd.frame->deleteLater();
    }
    m_chips.clear();
    updateVisibility();
}

void FilterBarWidget::updateVisibility()
{
    setVisible(!m_chips.isEmpty());
    updateCloseButtons();
}

void FilterBarWidget::updateCloseButtons()
{
    const int last = m_chips.size() - 1;
    for (int i = 0; i <= last; ++i) {
        const bool isLast = (i == last);
        m_chips[i].closeBtn->setEnabled(isLast);
        m_chips[i].closeBtn->setToolTip(
            isLast ? tr("Remove this filter")
                   : tr("Remove the most recently applied filter first"));
    }
}
