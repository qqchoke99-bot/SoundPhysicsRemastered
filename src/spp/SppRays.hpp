#pragma once
// SPP player-cast ray system — port of RaycastingHelper.java (MIT, FalseMSP/RedDev).
// White rays bounce from the listener (Fibonacci sphere, 200 dirs, 3 bounces).
// Blue rays probe reverb back to the player; red rays do permeation per sound.
// Bedrock adaptation: world.clip -> mc::isSolid stepping; Java thread pools ->
// single-threaded chunked probe (same math, no thread explosion on mobile).
#include "fmod/FmodApi.hpp"
#include <vector>

namespace spz::spp {

struct Vec3 {
    double x = 0, y = 0, z = 0;
    Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 scale(double s) const { return {x * s, y * s, z * s}; }
    double dot(const Vec3& o) const { return x * o.x + y * o.y + z * o.z; }
    double length() const;
    Vec3 normalize() const;
    double distanceTo(const Vec3& o) const;
};

// Reverb accumulators (one probe). Mirrors RaycastingHelper atomics.
struct SppProbe {
    double distWallSum = 0;
    double distWallDenom = 0;
    double reverbStrength = 0;      // blue rays that made it back
    double reverbDenom = 0;
    double weightedReverb = 0;      // reflection-quality weighted
    double outdoorLeak = 0;         // segments that missed everything
    double outdoorLeakDenom = 0;
    double totalRaysHitSurface = 0;
    double earlyStrength = 0;
    double earlyCount = 0;
    double lateStrength = 0;
    double lateCount = 0;
    double avgAbsorption = 0;       // distance-weighted absorption sum
    double totalSurface = 0;        // bounce count proxy for surface area
};

// Enhanced reverb (Sabine). Mirrors calculateEnhancedReverb().
struct SppRoom {
    double rt60 = 0;
    double earlyDelay = 0.02;   // seconds
    double lateStrength = 0;
    double roomRadius = 8;      // blocks (sphere-equivalent)
    double absorption = 0.05;
    bool isIndoors = false;
};

// Per-channel SPP result for occlusion + directional volume.
struct SppChannel {
    double weight = 0;            // averaged ray weight
    double distance = 8;          // averaged distance
    double permeation = 1;        // 0..1 muffle factor
    double confidence = 0;        // rayCount / totalRaysHitSurface
    double attenuation = 1;       // 1/d (VERCIDIUM_LINEAR default)
    double occlusion = 0;         // 0..1 direct blockage for FMOD
    Vec3 dirFromListener{0, 0, 1};
};

// Full probe: cast all white rays from eye, update stats + per-channel data.
// sounds: listener-relative? No — absolute positions of active channels.
// Returns room model; fills channels in place. Call every kTickRate ticks.
struct SppStats {
    SppProbe accum;
    SppRoom room;
};
SppStats probeFull(const Vec3& eye, const Vec3& playerCenter,
                   std::vector<std::pair<Vec3, SppChannel*>> sounds);

// Back-compat: room only.
inline SppRoom probe(const Vec3& eye, const Vec3& playerCenter,
                     std::vector<std::pair<Vec3, SppChannel*>> sounds) {
    return probeFull(eye, playerCenter, sounds).room;
}

std::vector<Vec3> fibonacciSphere(int n);
Vec3 reflectVec(const Vec3& incident, const Vec3& normal);

// Test seam: override rays for unit checks.
void setRayOverride(const std::vector<Vec3>& dirs);
void clearRayOverride();

} // namespace spz::spp
