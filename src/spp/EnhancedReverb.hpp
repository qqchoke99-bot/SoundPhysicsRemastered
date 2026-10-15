#pragma once
#include "spp/Config.hpp"
#include <string>

namespace spp {

// Port of EnhancedReverbData + calculateEnhancedReverb
struct EnhancedReverb {
    double rt60 = 0.3;                 // seconds
    double earlyReflectionDelay = 0.02; // seconds
    double lateReflectionStrength = 0;
    double roomSize = 1;
    double absorption = 0.05;
    std::string acousticProfile = "generic_room";
    bool isIndoors = false;
};

// Ray accumulation input (filled by probe — same roles as SPP atomics)
struct SurfaceAccum {
    double totalSurfaceArea = 0;      // sum of hit weights / areas
    double averageAbsorption = 0;     // sum of (area * alpha) before divide
    double distanceFromWallSum = 0;
    double distanceFromWallCount = 0;
    double surfaceToVolumeRatio = 0;
    int earlyReflectionCount = 0;
    double earlyReflectionStrength = 0;
    int lateReflectionCount = 0;
    double lateReflectionStrength = 0;
};

EnhancedReverb calculateEnhancedReverb(const SurfaceAccum& a);

// OpenAL EFX-style outputs mapped later to FMOD_REVERB_PROPERTIES
struct EfxParams {
    float decayTimeSec = 0.5f;
    float reflectionsDelaySec = 0.02f;
    float lateReverbDelaySec = 0.03f;
    float gain = 0.3f;           // linear-ish 0..1
    float gainHF = 0.8f;
    float reflectionsGain = 0.3f;
    float lateReverbGain = 0.4f;
    float diffusion = 0.8f;
    float density = 0.8f;
    float roomRolloff = 0.0f;
};

EfxParams toEfx(const EnhancedReverb& e, const cfg::Settings& s);

}
