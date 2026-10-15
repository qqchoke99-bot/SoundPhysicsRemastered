// SPP reverb math — line-for-line port of OpenALEffectsHandler.java
// applyReverbToSource / applyLegacyReverbToSource (MIT, FalseMSP/RedDev).
// Only the actuation differs: EAX alEffectf -> FMOD_REVERB_PROPERTIES.
#include "spp/SppReverb.hpp"
#include "spp/SppConfig.hpp"

#include <algorithm>
#include <cmath>

namespace spz::spp {
namespace {

float clampf(float v, float lo, float hi) { return std::min(std::max(v, lo), hi); }
float lerpf(float a, float b, float t) { return a + (b - a) * t; }

// ReverbConstants.java clamps
constexpr float kMinWall = 0.5f, kMaxWall = 200.0f;
constexpr float kMinRt = 0.1f, kMaxRt = 10.0f;
constexpr float kMinCalcRt = 0.1f, kMaxCalcRt = 8.0f;
constexpr float kMinRoom = 1.0f, kMaxRoom = 100.0f;
constexpr float kMinAbs = 0.01f, kMaxAbs = 0.9f;
constexpr float kMinEarly = 0.001f, kMaxEarly = 0.1f;
constexpr float kMinLate = 0.005f, kMaxLate = 0.1f;

SppFmod outdoor() {
    SppFmod f;
    f.decayMs = 180;
    f.wetDb = -80;
    f.echoWetDb = -80;
    f.echoFeedback = 0;
    f.enclosure = 0;
    f.indoors = false;
    return f;
}

} // namespace

SppFmod computeReverb(const SppStats& stats) {
    const SppProbe& p = stats.accum;
    const SppRoom& rd = stats.room;
    if (p.distWallDenom == 0 || p.reverbDenom == 0 || p.outdoorLeakDenom == 0)
        return outdoor();

    float wallDistance = (float)(p.distWallSum / p.distWallDenom);
    float reverbStrength = (float)(p.reverbStrength / p.reverbDenom);
    float weighted = (float)(p.weightedReverb / p.reverbDenom);
    float leak = (float)(p.outdoorLeak / p.outdoorLeakDenom);
    float earlyRatio = (p.earlyCount + p.lateCount) > 0
        ? (float)(p.earlyCount / (p.earlyCount + p.lateCount)) : 0;

    float rt60 = (float)rd.rt60;
    float roomSize = (float)rd.roomRadius;
    float absorption = (float)rd.absorption;
    float earlyDelay = (float)rd.earlyDelay;
    float lateStrength = (float)rd.lateStrength;

    wallDistance = clampf(wallDistance, kMinWall, kMaxWall);
    reverbStrength = clampf(reverbStrength, 0, 1);
    weighted = clampf(weighted, 0, 1);
    leak = clampf(leak, 0, 1);
    earlyRatio = clampf(earlyRatio, 0, 1);
    roomSize = clampf(roomSize, kMinRoom, kMaxRoom);
    absorption = clampf(absorption, kMinAbs, kMaxAbs);
    earlyDelay = clampf(earlyDelay, kMinEarly, kMaxEarly);
    lateStrength = clampf(lateStrength, 0, 1);

    float enclosure = rd.isIndoors ? (1.0f - leak) * kIndoorBias
                                   : std::max(0.0f, reverbStrength - leak) * kOutdoorBias;
    enclosure = clampf(enclosure, 0, 1);
    float openness = 1.0f - enclosure;

    float estVol = roomSize * roomSize * roomSize * kRoomVolumeMult;
    float estSA = kSurfaceAreaMult * roomSize * roomSize;
    float svRatio = clampf(estSA / std::max(estVol, 1.0f), 0.1f, 5.0f);

    float totalAbs = estSA * absorption;
    float calcRt = rt60;
    if (calcRt < kMinRt || calcRt > kMaxRt) {
        calcRt = kRt60Sabine * estVol / std::max(totalAbs, kMinTotalAbsorption);
        calcRt = clampf(calcRt, kMinCalcRt, kMaxCalcRt);
    }
    float decay = clampf(calcRt * enclosure, 0.1f, 8.0f);

    float distAtten = 1.0f / (1.0f + wallDistance * kDistAttenLinear +
                              wallDistance * wallDistance * kDistAttenQuad);
    float airAbs = clampf(1.0f - wallDistance * kAirAbsorptionRate, kMinAirAbsorption, 1.0f);
    float decayHf = clampf((1.0f - absorption) * airAbs * enclosure, 0.1f, 2.0f);

    float reflGain = clampf(earlyRatio * reverbStrength * enclosure * distAtten * kGlobalReverbIntensity, 0, 3);
    float lateDelay = clampf(earlyDelay * kLateDelayMult + roomSize / kSoundSpeed, kMinLate, kMaxLate);
    float lateGain = clampf(lateStrength * enclosure * distAtten * kGlobalReverbIntensity, 0, 10);
    float complexity = std::min(1.0f, svRatio / kRoomComplexityDiv);

    // room-size scaling tiers (exact SPP thresholds)
    float sizeScale = 1, smallRed = 1, volRed = 1;
    if (roomSize < 0.5f) { sizeScale = 0.05f; smallRed = 0.1f; volRed = 0.1f; }
    else if (roomSize < 2.0f) { sizeScale = 0.15f; smallRed = 0.25f; volRed = 0.3f; }
    else if (roomSize < 5.0f) { sizeScale = 0.4f; smallRed = 0.5f; volRed = 0.6f; }
    else if (roomSize < 20.0f) { sizeScale = 0.7f; smallRed = 0.8f; volRed = 0.8f; }
    volRed *= std::max(0.1f, std::min(1.0f, estVol / 500.0f));
    float emphasis = roomSize < 10.0f ? kSmallRoomEmphasis * smallRed : kLargeRoomEmphasis;

    float diffusion = lerpf(kMinDiffusion, kMaxDiffusion,
        complexity * kDiffComplexW * enclosure * kDiffEnclW * weighted * kDiffReverbW * emphasis);
    diffusion = clampf(diffusion, 0.1f, 1.0f);
    float density = lerpf(kMinDensity, 1.0f,
        enclosure * (1.0f - roomSize / kDensityRoomSizeFactor) * emphasis);
    density = clampf(density, 0.1f, 1.0f);

    float overall = enclosure * distAtten * sizeScale * volRed *
        (kBaseReverbGain + reverbStrength * kReverbGainMult) * kGlobalReverbIntensity;
    overall = clampf(overall, 0, kMaxOverallGain);

    reflGain = clampf(reflGain * sizeScale, 0, 3);
    lateGain = clampf(lateGain * sizeScale, 0, 10);
    decay = clampf(decay * sizeScale, 0.1f, 8.0f);

    SppFmod f;
    f.decayMs = decay * 1000.0f;
    f.earlyMs = earlyDelay * 1000.0f;
    f.lateMs = lateDelay * 1000.0f;
    f.wetDb = overall <= 1e-4f ? -80.0f : std::max(-80.0f, 20.0f * std::log10(overall));
    f.diffusion = diffusion;
    f.density = density;
    f.hfRatio = decayHf;
    f.hfRef = clampf(kBaseHfRef - roomSize * kHfRoomSizeFactor, 1000.0f, 20000.0f);
    f.echoDelayMs = clampf(roomSize * kEchoTimeMult * 1000.0f, 75.0f, 250.0f);
    f.echoFeedback = clampf(lerpf(1.0f, 0.4f, roomSize / kDensityRoomSizeFactor) *
                            enclosure * 0.5f, 0.0f, 0.55f);
    f.echoWetDb = std::max(-28.0f, f.wetDb - 6.0f);
    f.enclosure = enclosure;
    f.indoors = rd.isIndoors;
    (void)lateGain; (void)reflGain; (void)openness; // EAX-only sends; FMOD packs into wet
    return f;
}

SppFmod computeLegacy(const SppStats& stats) {
    const SppProbe& p = stats.accum;
    if (p.distWallDenom == 0 || p.reverbDenom == 0 || p.outdoorLeakDenom == 0)
        return outdoor();
    float wall = clampf((float)(p.distWallSum / p.distWallDenom), 1.0f, 100.0f);
    float occ = 1.0f - clampf((float)(p.reverbStrength / p.reverbDenom) +
                              (float)(p.outdoorLeak / p.outdoorLeakDenom), 0.0f, 1.0f);
    float leak = clampf((float)(p.outdoorLeak / p.outdoorLeakDenom), 0.0f, 1.0f);
    float dry = 1.0f - leak;
    float delay = (wall * 2.0f) / 343.0f; // seconds, round trip
    SppFmod f;
    f.decayMs = clampf(delay * 5.0f * dry, 0.1f, 6.0f) * 1000.0f;
    f.earlyMs = clampf(delay * 0.5f, 0.005f, 0.05f) * 1000.0f;
    f.lateMs = clampf(delay, 0.01f, 0.1f) * 1000.0f;
    f.diffusion = lerpf(0.3f, 1.0f, dry * (1.0f - occ));
    f.density = lerpf(0.3f, 1.0f, dry);
    float gain = lerpf(0.05f, 0.3f, dry);
    f.wetDb = gain <= 1e-4f ? -80.0f : 20.0f * std::log10(gain);
    f.hfRatio = lerpf(0.5f, 1.3f, (1.0f - occ) * dry);
    f.echoDelayMs = f.lateMs;
    f.echoFeedback = 0.2f * dry;
    f.echoWetDb = f.wetDb - 3.0f;
    f.enclosure = dry;
    f.indoors = dry > 0.5f;
    return f;
}

} // namespace spz::spp
