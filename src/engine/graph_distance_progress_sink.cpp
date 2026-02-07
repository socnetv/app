#include "graph_distance_progress_sink.h"
#include "../graph.h"   // adjust include if you prefer "graph.h" from include paths

GraphDistanceProgressSink::GraphDistanceProgressSink(Graph& g)
    : graph(g)
{}

void GraphDistanceProgressSink::statusMessage(const QString& msg)
{
    emit graph.statusMessage(msg);
}

void GraphDistanceProgressSink::progressCreate(const int total, const QString& msg)
{
    emit graph.signalProgressBoxCreate(total, msg);
}

void GraphDistanceProgressSink::progressUpdate(const int value)
{
    emit graph.signalProgressBoxUpdate(value);
}

void GraphDistanceProgressSink::progressKill()
{
    emit graph.signalProgressBoxKill();
}
