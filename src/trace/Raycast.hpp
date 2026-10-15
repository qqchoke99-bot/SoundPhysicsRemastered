#pragma once
// Specular ray tracing: DDA through blocks, reflect off faces (Sound Physics style).
#include "fmod/FmodApi.hpp"
#include <vector>

namespace spz::trace {

struct RayHit {
    bool hit = false;
    float distance = 0;
    int bx = 0, by = 0, bz = 0;
    FMOD_VECTOR normal{0, 1, 0};
    FMOD_VECTOR point{};
};

struct SpecularArrival {
    float delayMs = 0;     // path length / speedOfSound * 1000
    float gain = 0;        // energy after absorption / distance
    float pathLen = 0;
    int order = 0;         // bounce count
};

struct RaycastConfig {
    int rays = 12;             // directions from listener or source
    int maxBounces = 2;        // specular reflections after first hit
    float maxDistance = 24.f;
    float speedOfSound = 343.f; // blocks/s
    float reflectivity = 0.85f;
    float airAbsorption = 0.006f;
};

// Single primary ray + optional specular bounces. Uses mc::isSolid / blockCost.
void traceSpecular(const FMOD_VECTOR& origin,
                   const FMOD_VECTOR& listener,
                   const RaycastConfig& cfg,
                   std::vector<SpecularArrival>& out);

// Enclosure probe: N directions, returns hit fraction + mean distance (for room size).
struct EnclosureProbe {
    float enclosure = 0;
    float meanDist = 8;
    float ceilDist = 16;
    float floorDist = 16;
    int sky = 0;
};
EnclosureProbe probeEnclosure(const FMOD_VECTOR& origin, float radius, int dirs = 12);

}
