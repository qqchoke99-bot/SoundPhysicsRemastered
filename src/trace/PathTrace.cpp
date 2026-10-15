#include "trace/PathTrace.hpp"
#include "mc/World.hpp"
#include <algorithm>
#include <cmath>
#include <vector>

namespace spz::trace {
namespace {

// Deterministic "random" from index (no RNG needed on audio thread)
void scatterDir(int seed, float nx, float ny, float nz, float& dx, float& dy, float& dz) {
    // Hemisphere around normal using golden seeds
    float u = std::fmod(seed * 0.618033f, 1.f);
    float v = std::fmod(seed * 0.381966f, 1.f);
    float th = 2.f * 3.14159265f * u;
    float ct = std::sqrt(std::max(0.f, 1.f - v));
    float st = std::sqrt(std::max(0.f, v));
    // Build tangent basis
    FMOD_VECTOR t = (std::fabs(ny) < 0.9f) ? FMOD_VECTOR{0,1,0} : FMOD_VECTOR{1,0,0};
    float cx = ny*t.z - nz*t.y, cy = nz*t.x - nx*t.z, cz = nx*t.y - ny*t.x;
    float cl = std::sqrt(cx*cx+cy*cy+cz*cz)+1e-6f;
    cx/=cl; cy/=cl; cz/=cl;
    float bx = cy*nz - cz*ny, by = cz*nx - cx*nz, bz = cx*ny - cy*nx;
    dx = st*std::cos(th)*cx + st*std::sin(th)*bx + ct*nx;
    dy = st*std::cos(th)*cy + st*std::sin(th)*by + ct*ny;
    dz = st*std::cos(th)*cz + st*std::sin(th)*bz + ct*nz;
}

} // namespace

PathEnergy traceDiffuse(const FMOD_VECTOR& listener, const PathTraceConfig& cfg) {
    PathEnergy e;
    const int P = std::max(4, std::min(24, cfg.paths));
    float gainSum = 0, delaySum = 0, delaySq = 0;
    int n = 0;

    for (int p = 0; p < P; ++p) {
        float z = 1.f - 2.f * (p + 0.5f) / P;
        float rr = std::sqrt(std::max(0.f, 1.f - z*z));
        float th = 2.399963f * p;
        float dx = rr*std::cos(th), dy = z, dz = rr*std::sin(th);
        FMOD_VECTOR pos = listener;
        float path = 0;
        float energy = 1.f;

        for (int depth = 0; depth < cfg.scatterDepth; ++depth) {
            // march
            float step = 0.3f, t = step, maxD = cfg.maxSegment;
            bool hit = false;
            float hx=0,hy=0,hz=0, nx=0,ny=1,nz=0;
            while (t <= maxD) {
                int bx=(int)std::floor(pos.x+dx*t);
                int by=(int)std::floor(pos.y+dy*t);
                int bz=(int)std::floor(pos.z+dz*t);
                if (mc::isSolid(bx,by,bz)) {
                    hit = true;
                    hx=pos.x+dx*t; hy=pos.y+dy*t; hz=pos.z+dz*t;
                    // crude normal
                    nx = dx>0?-1.f:1.f; ny=0; nz=0;
                    if (std::fabs(dy)>=std::fabs(dx)&&std::fabs(dy)>=std::fabs(dz)) {
                        nx=0; ny=dy>0?-1.f:1.f; nz=0;
                    } else if (std::fabs(dz)>=std::fabs(dx)) {
                        nx=0; ny=0; nz=dz>0?-1.f:1.f;
                    }
                    path += t;
                    break;
                }
                t += step;
            }
            if (!hit) break;
            energy *= cfg.reflectivity * std::exp(-cfg.airAbsorption * t);
            if (energy < 0.03f) break;

            float delayMs = 1000.f * path / std::max(1.f, cfg.speedOfSound);
            gainSum += energy;
            delaySum += delayMs;
            delaySq += delayMs * delayMs;
            ++n;

            scatterDir(p * 17 + depth * 3, nx, ny, nz, dx, dy, dz);
            pos = {hx + dx*0.08f, hy + dy*0.08f, hz + dz*0.08f};
        }
    }

    e.pathsHit = n;
    if (n > 0) {
        e.lateGain = std::min(1.f, gainSum / P);
        e.meanDelayMs = delaySum / n;
        float mean = e.meanDelayMs;
        e.spreadMs = std::sqrt(std::max(0.f, delaySq / n - mean * mean));
    }
    return e;
}

HybridResult hybridRoom(const FMOD_VECTOR& listener,
                        const RaycastConfig& rayCfg,
                        const PathTraceConfig& pathCfg,
                        float maxRt60,
                        float maxWetDb) {
    HybridResult r;
    r.room = probeEnclosure(listener, rayCfg.maxDistance, rayCfg.rays);

    std::vector<SpecularArrival> specs;
    traceSpecular(listener, listener, rayCfg, specs);

    PathEnergy dif = traceDiffuse(listener, pathCfg);

    // Room RT60 from mean distance (Sabine proxy) + diffuse energy
    float V = r.room.meanDist * r.room.meanDist * r.room.meanDist;
    float S = std::max(1.f, 6.f * r.room.meanDist * r.room.meanDist);
    float rt = 0.161f * V / (0.15f * S);
    rt *= (0.7f + 0.6f * dif.lateGain);
    rt = std::max(0.12f, std::min(rt, maxRt60));
    r.decayMs = rt * 1000.f;

    if (r.room.sky || r.room.enclosure < 0.22f) {
        r.wetDb = -80.f;
        r.decayMs = 180.f;
        r.echoWetDb = -80.f;
        r.echoFeedback = 0.f;
    } else {
        float energy = std::max(r.room.enclosure, dif.lateGain);
        energy = std::pow(std::max(0.f, energy), 0.55f);
        r.wetDb = -12.f + (1.f - energy) * -18.f;
        r.wetDb = std::max(-24.f, std::min(r.wetDb, maxWetDb));

        // Specular echo: earliest strong arrival
        float bestG = 0, bestD = 40.f;
        for (auto& a : specs) {
            if (a.gain > bestG && a.order <= 2) {
                bestG = a.gain;
                bestD = a.delayMs;
            }
        }
        r.echoDelayMs = std::max(8.f, std::min(bestD, 200.f));
        r.echoFeedback = std::min(0.55f, 0.15f + bestG * 0.5f + dif.lateGain * 0.2f);
        r.echoWetDb = std::max(-28.f, r.wetDb - 6.f);
    }

    r.earlyMs = std::max(5.f, std::min(r.room.meanDist / 17.15f * 1000.f, 70.f));
    r.diffusion = std::min(1.f, 0.4f + dif.lateGain * 0.5f + (dif.spreadMs / 80.f) * 0.2f);
    r.density = std::min(1.f, 0.45f + r.room.enclosure * 0.4f);
    return r;
}

} // namespace spz::trace
