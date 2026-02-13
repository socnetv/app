/**
 * @file distance_progress_sink.h
 * @brief Declares the IDistanceProgressSink interface for receiving progress updates during distance and centrality computations.
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


#pragma once

#include <QString>

class IDistanceProgressSink
{
public:
    virtual ~IDistanceProgressSink() = default;

    virtual void statusMessage(const QString& msg) = 0;
    virtual void progressCreate(int total, const QString& msg) = 0;
    virtual void progressUpdate(int value) = 0;
    virtual void progressKill() = 0;
};
