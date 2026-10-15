#pragma once
// Diffuse path tracing: at each hit, scatter into a new random-ish direction
// (ensemble measures late reverb energy). Complements specular Raycast.
#include "fmod/FmodApi.hpp"
#include "trace/Raycast.hpp"

namespace spz::trace {

struct PathTraceConfig {
    int paths = 12;            // independent walkers
    int scatterDepth = 3;      // segments per path (first hit + scatters)
    float maxSegment = 20.f;
    float speedOfSound = 343.f;
    float reflectivity = 0.85f;
    float airAbsorption = 0.006f;
    float scatter = 1.f;       // 1 = full diffuse; 0 = prefer specular (unused mix)
    float specularMix = 0.35f; // blend factor with specular arrivals
};

struct PathEnergy {
    float lateGain = 0;        // diffuse tail strength 0..1
    float meanDelayMs = 40;
    float spreadMs = 20;       // variance cue for diffusion
    int pathsHit = 0;
};

// From listener: shoot paths, accumulate energy that could return (simple field probe).
PathEnergy traceDiffuse(const FMOD_VECTOR& listener, const PathTraceConfig& cfg);

// Merge specular arrivals + diffuse field → echo delay / feedback / wet boost.
struct HybridResult {
    float decayMs = 800;
    float wetDb = -18;
    float earlyMs = 25;
    float echoDelayMs = 40;
    float echoFeedback = 0.2f;
    float echoWetDb = -20;
    float diffusion = 0.7f;
    float density = 0.7f;
    EnclosureProbe room{};
};

HybridResult hybridRoom(const FMOD_VECTOR& listener,
                        const RaycastConfig& rayCfg,
                        const PathTraceConfig& pathCfg,
                        float maxRt60,
                        float maxWetDb);

}
