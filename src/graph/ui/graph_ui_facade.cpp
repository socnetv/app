/**
 * @file graph_ui_facade.cpp
 * @brief Implements Façade wrapper methods called by the UI

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

#include "graph.h"
#include <QThread>

QThread* Graph::getThread() const {
    return thread();
}

void Graph::moveToThreadFacade(QThread *t) {
    moveToThread(t);
}
