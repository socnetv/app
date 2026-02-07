#pragma once

#include "distance_progress_sink.h"

class Graph; // forward declaration

class GraphDistanceProgressSink final : public IDistanceProgressSink
{
public:
    explicit GraphDistanceProgressSink(Graph& g);

    void statusMessage(const QString& msg) override;
    void progressCreate(int total, const QString& msg) override;
    void progressUpdate(int value) override;
    void progressKill() override;

private:
    Graph& graph;
};
