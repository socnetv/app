/**
 * @file null_distance_progress_sink.h
 * @brief Declares the NullDistanceProgressSink class that implements IDistanceProgressSink with empty methods, for use when no UI updates are needed during distance and centrality computations.
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

#include "distance_progress_sink.h"

class NullDistanceProgressSink final : public IDistanceProgressSink
{
public:
    void statusMessage(const QString&) override {}
    void progressCreate(int, const QString&) override {}
    void progressUpdate(int) override {}
    void progressKill() override {}
    bool progressCanceled() const override { return false; }
};
