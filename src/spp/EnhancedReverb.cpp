#include "spp/EnhancedReverb.hpp"
#include "spp/ReverbConstants.hpp"
#include "spp/Config.hpp"
#include <algorithm>
#include <cmath>

namespace spp {
namespace {

double clampd(double v, double lo, double hi) {
    return std::max(lo, std::min(v, hi));
}

std::string determineAcousticProfile(double absorption, double roomRadius, double /*sToV*/) {
    // Port of determineAcousticProfile(double, double, double)
    if (roomRadius * roomRadius * roomRadius > 10000.0) return "cathedral";
    if (absorption > 0.6) return "padded_room";
    if (absorption < 0.1 && roomRadius < 0.3) return "gymnasium";
    if (roomRadius > 1.0) return "small_room"; // SPP compares room size threshold
    if (absorption > 0.3) return "living_room";
    return "generic_room";
}

} // namespace

EnhancedReverb calculateEnhancedReverb(const SurfaceAccum& a) {
    EnhancedReverb e;

    // alpha = averageAbsorption / totalSurfaceArea  (SPP) else 0.05
    double alpha = 0.05;
    if (a.totalSurfaceArea > 0.0)
        alpha = a.averageAbsorption / a.totalSurfaceArea;
    alpha = clampd(alpha, constants::MIN_ABSORPTION, constants::MAX_ABSORPTION);
    e.absorption = alpha;

    // avgWallDistance
    double avgWall = 8.0;
    if (a.distanceFromWallCount > 0.0)
        avgWall = a.distanceFromWallSum / a.distanceFromWallCount;
    avgWall = clampd(avgWall, constants::MIN_WALL_DISTANCE, constants::MAX_WALL_DISTANCE);

    // SPP uses pow(avgWall, 3) as volume proxy in the RT60 line
    const double volumeProxy = avgWall * avgWall * avgWall;
    const double totalAbs = a.totalSurfaceArea > 0.0 ? (a.totalSurfaceArea * alpha) : (alpha * 6.0 * avgWall * avgWall);

    // RT60 = 0.161 * avgWall^3 / totalAbsorption
    double rt60 = 0.0;
    if (totalAbs > 0.0)
        rt60 = constants::SABINE * volumeProxy / totalAbs;
    rt60 = clampd(rt60, constants::MIN_CALCULATED_RT60, constants::MAX_CALCULATED_RT60);
    e.rt60 = rt60;

    // roomRadius = cbrt(avgWall^3 * 3 / (4π))
    const double roomRadius = std::cbrt(volumeProxy * 3.0 / constants::FOUR_PI);
    e.roomSize = clampd(roomRadius, constants::MIN_ROOM_SIZE, constants::MAX_ROOM_SIZE);

    // earlyReflectionDelay = roomRadius / 17.15  (seconds)
    e.earlyReflectionDelay = clampd(roomRadius / constants::EARLY_DELAY_DIVISOR,
                                    constants::MIN_EARLY_REFLECTION_DELAY,
                                    constants::MAX_EARLY_REFLECTION_DELAY);

    // late strength from counts
    double late = 0.0;
    if (a.lateReflectionCount > 0)
        late = a.lateReflectionStrength / static_cast<double>(a.lateReflectionCount);
    e.lateReflectionStrength = late;

    e.acousticProfile = determineAcousticProfile(alpha, roomRadius, a.surfaceToVolumeRatio);
    // indoors: enough surface absorption area (SPP threshold style)
    e.isIndoors = (a.totalSurfaceArea > 0.0 && alpha >= 0.05 && avgWall < constants::MAX_WALL_DISTANCE * 0.5);

    return e;
}

EfxParams toEfx(const EnhancedReverb& e, const cfg::Settings& s) {
    EfxParams p;
    double rt = e.rt60;
    // large / small room emphasis
    if (e.roomSize > 8.0)
        rt *= s.largeRoomEmphasis;
    else
        rt *= s.smallRoomEmphasis;
    rt = clampd(rt, constants::MIN_RT60, s.maxRt60Seconds);

    p.decayTimeSec = static_cast<float>(rt);
    p.reflectionsDelaySec = static_cast<float>(e.earlyReflectionDelay);
    p.lateReverbDelaySec = static_cast<float>(e.earlyReflectionDelay * 1.5 + 0.01);

    double indoor = e.isIndoors ? s.indoorBias : (1.0 / std::max(1.0, (double)s.outdoorBias));
    double gain = s.baseReverbGain * s.reverbGainMultiplier * s.globalReverbIntensity * indoor;
    gain *= (0.35 + 0.65 * std::min(1.0, e.lateReflectionStrength + (e.isIndoors ? 0.5 : 0.0)));
    gain = clampd(gain, 0.0, s.maxOverallGain);
    p.gain = static_cast<float>(gain);
    p.reflectionsGain = p.gain * 0.7f;
    p.lateReverbGain = p.gain;

    p.diffusion = std::max(s.minDiffusion, std::min(s.maxDiffusion, 0.5f + 0.4f * (e.isIndoors ? 1.f : 0.2f)));
    p.density = std::max(s.minDensity, std::min(1.f, s.minDensity + s.densityRoomSizeFactor * (float)(e.roomSize / 20.0)));

    // absorption → HF
    p.gainHF = static_cast<float>(clampd(1.0 - e.absorption, 0.1, 1.0));
    if (!e.isIndoors) {
        p.gain *= 0.05f;
        p.decayTimeSec = std::min(p.decayTimeSec, 0.25f);
    }
    return p;
}

} // namespace spp
