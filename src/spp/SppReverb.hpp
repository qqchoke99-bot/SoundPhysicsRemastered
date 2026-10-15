#pragma once
// SPP reverb -> FMOD mapper. Ports OpenALEffectsHandler::applyReverbToSource
// (MIT, FalseMSP/RedDev) EAX math; outputs FMOD-native reverb properties.
// OpenAL EAX has 20+ knobs, FMOD has 12 — decay/diffusion/density/wet/early/
// late/HF map 1:1, echo+modulation are computed for a future Echo DSP.
#include "spp/SppRays.hpp"

namespace spz::spp {

struct SppFmod {
    float decayMs = 800;     // FMOD DecayTime
    float earlyMs = 20;       // FMOD EarlyDelay
    float lateMs = 35;        // FMOD LateDelay
    float wetDb = -18;        // FMOD WetLevel
    float diffusion = 0.7f;   // 0..1 (x100 for FMOD)
    float density = 0.7f;     // 0..1 (x100 for FMOD)
    float hfRatio = 0.65f;    // FMOD HFDecayRatio
    float hfRef = 5000;       // FMOD HFReference
    float echoDelayMs = 40;   // future Echo DSP (SPP echoTime)
    float echoFeedback = 0.2f;
    float echoWetDb = -20;
    float enclosure = 0;      // 0 outdoor .. 1 indoors (debug/log)
    bool indoors = false;
};

// Needs >=1 sample or returns outdoor preset (mirrors the denom==0 early-out).
SppFmod computeReverb(const SppStats& stats);

// Legacy dry-factor path (applyLegacyReverbToSource). mode 0=MODERN skip.
SppFmod computeLegacy(const SppStats& stats);

} // namespace spz::spp
