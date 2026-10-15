#pragma once
// SPP (Sound Physics Perfected) tuning — ported from
// FalseMSP/SoundPhysicsPerfected Config.java (MIT, by RedDev).
// Fabric 26.3 defaults. Full-accuracy: raysCast=200, raysBounced=3.
namespace spz::spp {

// --- ray core ---
inline constexpr int kRaysCast = 200;
inline constexpr int kRaysBounced = 3;
inline constexpr int kMaxRayLengthChunks = 4;   // segment = 16 * this blocks
inline constexpr int kProcRange = 200;          // blocks
inline constexpr int kTickRate = 10;            // probe every N game ticks
inline constexpr bool kPermeation = true;
inline constexpr double kPermeationStep = 0.0625;
inline constexpr bool kUseExplosionResistance = true;
inline constexpr int kAttenuationType = 3;      // 0 INVERSE_SQUARE,1 LINEAR,2 VERCIDIUM_INV_SQ,3 VERCIDIUM_LINEAR
inline constexpr double kMaxBlocksPermeated = 3.0;
inline constexpr double kPermeationAbsorption = 0.4;
inline constexpr bool kBarrierAsAir = true;
inline constexpr double kBounceAbsorption = 0.9;
inline constexpr bool kShortcutDirectionality = true;
inline constexpr float kVolumeMultiplier = 1.0f;

// --- reverb global ---
inline constexpr bool kReverb = true;
inline constexpr float kGlobalReverbIntensity = 2.0f;
inline constexpr float kIndoorBias = 2.0f;
inline constexpr float kOutdoorBias = 0.2f;
inline constexpr float kSmallRoomEmphasis = 1.0f;
inline constexpr float kLargeRoomEmphasis = 1.0f;

// --- distance ---
inline constexpr float kDistAttenLinear = 0.0f;
inline constexpr float kDistAttenQuad = 0.0f;
inline constexpr float kAirAbsorptionRate = 0.003f;
inline constexpr float kMinAirAbsorption = 0.1f;

// --- room ---
inline constexpr float kRoomVolumeMult = 0.7f;
inline constexpr float kSurfaceAreaMult = 6.0f;
inline constexpr float kRoomComplexityDiv = 2.0f;

// --- timing ---
inline constexpr float kLateDelayMult = 1.5f;
inline constexpr float kSoundSpeed = 343.0f;
inline constexpr double kSpeedOfSoundTicks = 17.15; // blocks per gametick

// --- gain ---
inline constexpr float kBaseReverbGain = 0.1f;
inline constexpr float kReverbGainMult = 0.4f;
inline constexpr float kMaxOverallGain = 1.0f;
inline constexpr float kSendFilterHfReduction = 0.7f;

// --- diffusion/density ---
inline constexpr float kMinDiffusion = 0.3f;
inline constexpr float kMaxDiffusion = 0.95f;
inline constexpr float kMinDensity = 0.3f;
inline constexpr float kDensityRoomSizeFactor = 100.0f;

// --- freq ---
inline constexpr float kDynAbsLfFactor = 3.0f;
inline constexpr float kDecayLfMult = 1.2f;
inline constexpr float kOutdoorHfLeak = 0.7f;

// --- echo/mod ---
inline constexpr float kEchoTimeMult = 0.002f;
inline constexpr float kEchoDepthMult = 0.1f;
inline constexpr float kModTimeMult = 0.01f;
inline constexpr float kModDepthMult = 0.1f;

// --- refs ---
inline constexpr float kBaseHfRef = 5000.0f;
inline constexpr float kHfRoomSizeFactor = 20.0f;
inline constexpr float kBaseLfRef = 250.0f;
inline constexpr float kLfRoomSizeFactor = 2.0f;
inline constexpr float kRoomRolloffSizeFactor = 50.0f;

// --- sabine ---
inline constexpr float kRt60Sabine = 0.161f;
inline constexpr float kMinTotalAbsorption = 0.1f;

// --- interp weights ---
inline constexpr float kDiffComplexW = 1.0f;
inline constexpr float kDiffEnclW = 1.0f;
inline constexpr float kDiffReverbW = 1.0f;

} // namespace spz::spp
