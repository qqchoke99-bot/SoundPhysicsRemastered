#pragma once
// Subset of SPP Config (YACL) — only fields that feed calculateEnhancedReverb / EFX mapping
namespace spp::cfg {
struct Settings {
    bool reverb = true;
    float globalReverbIntensity = 1.0f;
    float indoorBias = 1.0f;
    float outdoorBias = 1.0f;
    float smallRoomEmphasis = 1.0f;
    float largeRoomEmphasis = 1.0f;
    float airAbsorptionRate = 0.01f;
    float minAirAbsorption = 0.05f;
    float roomVolumeMultiplier = 1.0f;
    float surfaceAreaMultiplier = 1.0f;
    float rt60SabineConstant = 0.161f;
    float minTotalAbsorption = 0.05f;
    float baseReverbGain = 0.5f;
    float reverbGainMultiplier = 1.0f;
    float maxOverallGain = 1.0f;
    float minDiffusion = 0.4f;
    float maxDiffusion = 1.0f;
    float minDensity = 0.4f;
    float densityRoomSizeFactor = 0.5f;
    float soundSpeed = 343.f; // blocks/s same scale as metres in SPP
    // User ceilings
    float maxRt60Seconds = 4.0f;
    float maxWetDb = -6.f;
};
Settings& settings();
}
