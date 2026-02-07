#pragma once

#include "distance_progress_sink.h"

class NullDistanceProgressSink final : public IDistanceProgressSink
{
public:
    void statusMessage(const QString&) override {}
    void progressCreate(int, const QString&) override {}
    void progressUpdate(int) override {}
    void progressKill() override {}
};
