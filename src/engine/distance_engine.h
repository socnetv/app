
#ifndef SOCNETV_DISTANCE_ENGINE_H
#define SOCNETV_DISTANCE_ENGINE_H

#include "engine/graph_distance_progress_sink.h"

class Graph;

class DistanceEngine
{
public:
    explicit DistanceEngine(Graph &g);
    void compute(const bool computeCentralities,
                 const bool considerWeights,
                 const bool inverseWeights,
                 const bool dropIsolates);

private:
    Graph &graph;

    // Keep helpers private; implemented in .cpp

    void initRun(const bool computeCentralities,
                 const bool considerWeights,
                 const bool inverseWeights,
                 const bool dropIsolates,
                 struct DistanceScratch &ds,
                 struct CentralityScratchSSSP &csssp,
                 struct CentralityScratchFinalize &csfin,
                 IDistanceProgressSink &sink);

    void runAllSources(const bool computeCentralities,
                       const bool considerWeights,
                       const bool inverseWeights,
                       const bool dropIsolates,
                       struct DistanceScratch &ds,
                       struct CentralityScratchSSSP &csssp,
                       IDistanceProgressSink &sink);

    void finalize(const bool computeCentralities,
                  const bool dropIsolates,
                  struct DistanceScratch &ds,
                  struct CentralityScratchFinalize &csfin,
                  IDistanceProgressSink &sink);
};

#endif // SOCNETV_DISTANCE_ENGINE_H
