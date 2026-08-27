/**
 * @file forms_logging.h
 * @brief Shared WS14 logging category declaration for src/forms/ dialog files.
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

#ifndef FORMS_LOGGING_H
#define FORMS_LOGGING_H

#include <QLoggingCategory>

// One shared category for all src/forms/ dialogs -- each file has too few qDebug() calls (1-16,
// pure one-shot UI code) to justify a category of its own; no pre-existing shared header exists
// across the ~20 dialog*.cpp files to piggyback on the way parser.h/graphicswidget.h did, so this
// small header exists purely to be that shared declaration point. Definition in
// dialogsettings.cpp (arbitrary single anchor, no dependency significance).
Q_DECLARE_LOGGING_CATEGORY(lcForms)

#endif // FORMS_LOGGING_H
