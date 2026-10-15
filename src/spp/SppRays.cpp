// SPP ray core — port of RaycastingHelper.java (MIT, FalseMSP/RedDev).
#include "spp/SppRays.hpp"
#include "spp/SppConfig.hpp"
#include "mc/World.hpp"

#include <cmath>
#include <algorithm>

namespace spz::spp {
namespace {

constexpr double kPi = 3.141592653589793;

struct ClipHit {
    bool hit = false;
    Vec3 pos{};
    Vec3 normal{0, 1, 0}; // face normal at hit
};

// March from o along d (normalized) up to maxDist using mc::isSolid.
// Face normal derived from previous voxel step (mirrors VCX/SPP face reflect).
ClipHit clip(const Vec3& o, const Vec3& d, double maxDist) {
    ClipHit r;
    double len = d.length();
    if (len < 1e-9) return r;
    Vec3 dir = d.scale(1.0 / len);
    constexpr double step = 0.5;
    double t = step;
    int pbx = (int)std::floor(o.x), pby = (int)std::floor(o.y), pbz = (int)std::floor(o.z);
    bool have = false;
    while (t <= maxDist) {
        int bx = (int)std::floor(o.x + dir.x * t);
        int by = (int)std::floor(o.y + dir.y * t);
        int bz = (int)std::floor(o.z + dir.z * t);
        // OPENCODE: barrier-as-air belongs in mc::isSolid when Bedrock ids wire up.
        if (mc::isSolid(bx, by, bz)) {
            r.hit = true;
            r.pos = {o.x + dir.x * t, o.y + dir.y * t, o.z + dir.z * t};
            if (have) {
                if (bx != pbx) r.normal = {(bx > pbx ? -1.0 : 1.0), 0, 0};
                else if (by != pby) r.normal = {0, (by > pby ? -1.0 : 1.0), 0};
                else r.normal = {0, 0, (bz > pbz ? -1.0 : 1.0)};
            }
            return r;
        }
        pbx = bx; pby = by; pbz = bz; have = true;
        t += step;
    }
    return r;
}

// OPENCODE: Bedrock block registry not wired yet — map blockCost (0..1)
// to SPP absorption. Solid -> 1.0 (stone-like: resistance 6/6), air -> 0.05
// default. Wire real explosionResistance/6 (cap 5) + dampens-vibration tags.
double absorptionAt(const Vec3& p) {
    int bx = (int)std::floor(p.x), by = (int)std::floor(p.y), bz = (int)std::floor(p.z);
    float cost = mc::blockCost(bx, by, bz);
    if (cost <= 0.0f) return 0.05;
    return 1.0;
}

// Port of countBlocksBetween (non-sublevel branch): walk start->end,
// accumulate in-block distance * absorption. Capped at maxBlocksPermeated.
double countBlocksBetween(const Vec3& start, const Vec3& end) {
    double total = 0;
    Vec3 cur = start;
    Vec3 full = end - start;
    double fullLen = full.length();
    if (fullLen < 1e-9) return 0;
    Vec3 dir = full.scale(1.0 / fullLen);
    int endBx = (int)std::floor(end.x), endBy = (int)std::floor(end.y), endBz = (int)std::floor(end.z);
    int guard = 0;
    while (total < kMaxBlocksPermeated && guard++ < 64) {
        double remain = (end - cur).length();
        if (remain <= 1e-6) break;
        ClipHit h = clip(cur, dir, remain);
        if (!h.hit) break;
        int hbx = (int)std::floor(h.pos.x), hby = (int)std::floor(h.pos.y), hbz = (int)std::floor(h.pos.z);
        if (hbx == endBx && hby == endBy && hbz == endBz) break;
        // exit point: step until outside this voxel (permeationStepSize)
        Vec3 exit = h.pos;
        int ex = 0;
        while ((int)std::floor(exit.x) == hbx && (int)std::floor(exit.y) == hby &&
               (int)std::floor(exit.z) == hbz && ex++ < 512) {
            exit = exit + dir.scale(kPermeationStep);
        }
        if ((cur - start).length() >= fullLen) break;
        double inBlock = h.pos.distanceTo(exit);
        total += inBlock * absorptionAt(h.pos);
        cur = exit + dir.scale(0.01);
    }
    return total;
}

double getWeight(double currentDistance, double blockCount, double distToEntity) {
    double permeationAbs = std::pow(kPermeationAbsorption, blockCount);
    double d = std::max(distToEntity + currentDistance, 0.1);
    switch (kAttenuationType) {
        case 0: return permeationAbs / (d * d);            // INVERSE_SQUARE
        case 1: return permeationAbs / d;                  // LINEAR
        case 2: return permeationAbs / (d * d);            // VERCIDIUM_INV_SQ
        case 3: return permeationAbs / (d * d);            // VERCIDIUM_LINEAR (sic: Java uses d^2 here too)
        default: return 0;
    }
}

struct SoundHit {
    double weight = 0;
    double muffle = 1;
    double dist = 0;
    Vec3 dir{0, 0, 1};
};

std::vector<Vec3> gOverride;
bool gHasOverride = false;

} // namespace

double Vec3::length() const { return std::sqrt(x * x + y * y + z * z); }
Vec3 Vec3::normalize() const {
    double l = length();
    if (l < 1e-9) return {0, 0, 1};
    return scale(1.0 / l);
}
double Vec3::distanceTo(const Vec3& o) const { return (*this - o).length(); }

Vec3 reflectVec(const Vec3& incident, const Vec3& normal) {
    double dot = incident.dot(normal);
    return incident - normal.scale(2.0 * dot);
}

std::vector<Vec3> fibonacciSphere(int n) {
    if (gHasOverride && !gOverride.empty()) return gOverride;
    n = std::max(1, n);
    std::vector<Vec3> dirs;
    dirs.reserve((size_t)n);
    double golden = (1.0 + std::sqrt(5.0)) / 2.0;
    for (int i = 0; i < n; ++i) {
        double theta = 2.0 * kPi * i / golden;
        double phi = std::acos(1.0 - 2.0 * (i + 0.5) / n);
        dirs.push_back({std::sin(phi) * std::cos(theta), std::cos(phi),
                        std::sin(phi) * std::sin(theta)});
    }
    return dirs;
}

void setRayOverride(const std::vector<Vec3>& dirs) { gOverride = dirs; gHasOverride = true; }
void clearRayOverride() { gOverride.clear(); gHasOverride = false; }

SppStats probeFull(const Vec3& eye, const Vec3& playerCenter,
                     std::vector<std::pair<Vec3, SppChannel*>> sounds) {
    SppProbe p;
    SppRoom room;
    double maxTotal = 16.0 * kMaxRayLengthChunks * kRaysBounced;
    double segLen = 16.0 * kMaxRayLengthChunks;
    std::vector<Vec3> dirs = fibonacciSphere(kRaysCast);

    // per-sound hit lists for weighted averaging (calculateWeightedAverages)
    std::vector<std::vector<SoundHit>> hits(sounds.size());

    for (const Vec3& rawDir : dirs) {
        Vec3 cur = eye;
        Vec3 curDir = rawDir.normalize();
        Vec3 initialDir = curDir;
        double remaining = maxTotal;
        double traveled = 0.0;

        auto redProbe = [&](const Vec3& from, double curDist) {
            for (size_t s = 0; s < sounds.size(); ++s) {
                const Vec3& tgt = sounds[s].first;
                double dEnt = from.distanceTo(tgt);
                if (dEnt + curDist > kMaxRayLengthChunks * 16) continue;
                double blocks = countBlocksBetween(from, tgt);
                double w = getWeight(curDist, blocks, dEnt);
                double perm = std::pow(kPermeationAbsorption, blocks);
                w *= perm;
                Vec3 n = (tgt - from).normalize();
                hits[s].push_back({w, perm, dEnt, n});
                (void)initialDir;
            }
        };

        redProbe(cur, traveled);

        for (int bounce = 0; bounce <= kRaysBounced && remaining > 0; ++bounce) {
            double seg = std::min(segLen, remaining);
            Vec3 segEnd = cur + curDir.scale(seg);
            ClipHit bh = clip(cur, curDir, seg);
            Vec3 actualEnd = bh.hit ? bh.pos : segEnd;
            double segTravel = cur.distanceTo(actualEnd);
            traveled += segTravel;
            traveled *= 1.0 / kBounceAbsorption;

            if (bh.hit) {
                // --- blue ray: reverb probe back to player ---
                if (kReverb && bounce <= 2) {
                    p.avgAbsorption += absorptionAt(bh.pos) * (1.0 / std::max(traveled, 1.0));
                    p.totalSurface += 1.0;
                }
                {
                    Vec3 toPlayer = (playerCenter - bh.pos).normalize();
                    Vec3 rs = bh.pos + toPlayer.scale(0.1);
                    double dEnt = rs.distanceTo(playerCenter);
                    ClipHit back = clip(rs, toPlayer, dEnt + 0.5);
                    bool los = !back.hit;
                    double delay = traveled / kSpeedOfSoundTicks;
                    if (delay < 0.05) {
                        p.earlyStrength += los ? 1.0 : 0.0;
                        p.earlyCount += 1.0;
                    } else {
                        p.lateStrength += los ? 0.6 : 0.0;
                        p.lateCount += 1.0;
                    }
                    p.reverbDenom += 1.0;
                    if (los) {
                        p.distWallSum += traveled;
                        p.distWallDenom += 1.0;
                        p.reverbStrength += 1.0;
                        Vec3 toAdj = rs - playerCenter;
                        Vec3 reflAngle = initialDir - toPlayer;
                        double quality = std::max(0.1, 1.0 - std::abs(reflAngle.length()));
                        p.weightedReverb += quality;
                        traveled = dEnt;
                        traveled *= 1.0 / kBounceAbsorption;
                        initialDir = toAdj.normalize();
                    }
                }
                redProbe(actualEnd, traveled);
            }

            if (bh.hit) {
                Vec3 refl = reflectVec(curDir, bh.normal);
                cur = bh.pos + refl.scale(0.01);
                curDir = refl;
                remaining -= segTravel;
                p.totalRaysHitSurface += 1.0;
                p.outdoorLeakDenom += 1.0;
            } else {
                Vec3 toCenter = (eye - actualEnd).normalize();
                curDir = reflectVec(curDir, toCenter);
                cur = segEnd + curDir.scale(0.01);
                remaining -= segTravel;
                p.outdoorLeak += 1.0;
                p.outdoorLeakDenom += 1.0;
                if (bounce == 0) break; // SPP: open sky, no bounce chain
            }
        }
    }

    // --- weighted averages per sound ---
    for (size_t s = 0; s < sounds.size(); ++s) {
        SppChannel* out = sounds[s].second;
        if (!out) continue;
        auto& list = hits[s];
        double tw = 0, wd = 0, wm = 0;
        Vec3 wdir{0, 0, 0};
        for (auto& h : list) {
            tw += h.weight;
            wd += h.dist * h.weight;
            wm += h.muffle * h.weight;
            wdir = wdir + h.dir.scale(h.weight);
        }
        // sounds behind walls still get red-ray hits (permeation); LOS shortcut
        // (shortcutDirectionality) is applied by the caller with real LOS.
        out->weight = tw;
        out->distance = tw > 0 ? wd / tw : (sounds[s].first - eye).length();
        out->permeation = tw > 0 ? wm / tw : 0.0;
        if (tw > 0) {
            double l = wdir.length();
            out->dirFromListener = l > 1e-9 ? wdir.scale(1.0 / l) : (sounds[s].first - eye).normalize();
        }
        double denom = std::max(p.totalRaysHitSurface, 1.0);
        out->confidence = (double)list.size() / denom;
        double d = std::max(out->distance, 0.01);
        out->attenuation = (kAttenuationType == 1) ? 1.0 / d : 1.0 / (d * d);
        // occlusion for FMOD: 1 - permeation (walls absorb direct path)
        double blocks = 0;
        if (out->permeation < 0.999) {
            // invert permeationAbsorption^blocks ~= permeation
            blocks = std::log(std::max(out->permeation, 1e-4)) / std::log(kPermeationAbsorption);
        }
        out->occlusion = std::min(1.0, std::max(0.0, (1.0 - out->permeation) * 0.9));
        (void)blocks;
    }

    // --- enhanced reverb (Sabine, cube-room assumption like SPP) ---
    double meanWall = p.distWallDenom > 0 ? p.distWallSum / p.distWallDenom : 0;
    double volume = meanWall * meanWall * meanWall;
    double roomRadius = std::cbrt(volume * 3.0 / (4.0 * kPi));
    double avgAbs = p.totalSurface > 0 ? p.avgAbsorption / p.totalSurface : 0.05;
    double totalAbs = p.totalSurface * avgAbs;
    double rt60 = totalAbs > 0 ? (kRt60Sabine * volume) / totalAbs : 0.0;
    room.rt60 = rt60;
    room.earlyDelay = roomRadius / kSpeedOfSoundTicks; // ticks->sec scale handled by caller
    double es = p.earlyCount > 0 ? p.earlyStrength / p.earlyCount : 0;
    double ls = p.lateCount > 0 ? p.lateStrength / p.lateCount : 0;
    (void)es;
    room.lateStrength = ls;
    room.roomRadius = roomRadius;
    room.absorption = avgAbs;
    double leakRatio = p.outdoorLeakDenom > 0 ? p.outdoorLeak / p.outdoorLeakDenom : 1.0;
    room.isIndoors = leakRatio < 0.05;
    return {p, room};
}

} // namespace spz::spp
