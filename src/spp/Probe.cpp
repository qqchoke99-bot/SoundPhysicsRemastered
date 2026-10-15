#include "spp/Probe.hpp"
#include <cmath>

namespace spp {

SurfaceAccum probeListener(const Vec3& origin, float maxDist, int rays, SampleFn sample) {
    SurfaceAccum a;
    if (!sample) return a;
    const int N = rays < 6 ? 6 : rays;
    int hits = 0;
    for (int i = 0; i < N; ++i) {
        float z = 1.f - 2.f * (i + 0.5f) / (float)N;
        float r = std::sqrt(std::max(0.f, 1.f - z * z));
        float th = 2.399963f * (float)i;
        float dx = r * std::cos(th), dy = z, dz = r * std::sin(th);
        float step = 0.35f;
        bool hit = false;
        float dist = maxDist;
        float alpha = 0.05f;
        for (float t = step; t <= maxDist; t += step) {
            int bx = (int)std::floor(origin.x + dx * t);
            int by = (int)std::floor(origin.y + dy * t);
            int bz = (int)std::floor(origin.z + dz * t);
            bool solid = false;
            float ab = 0.05f;
            sample(bx, by, bz, solid, ab);
            if (solid) {
                hit = true;
                dist = t;
                alpha = ab;
                break;
            }
        }
        if (hit) {
            ++hits;
            // area weight ~ 1 (SPP accumulates surface); use dist weight
            a.totalSurfaceArea += 1.0;
            a.averageAbsorption += alpha;
            a.distanceFromWallSum += dist;
            a.distanceFromWallCount += 1.0;
            if (dist < 12.0) {
                a.earlyReflectionCount++;
                a.earlyReflectionStrength += 1.0 / (1.0 + dist);
            } else {
                a.lateReflectionCount++;
                a.lateReflectionStrength += 0.5 / (1.0 + dist * 0.1);
            }
        }
    }
    if (a.distanceFromWallCount > 0) {
        double avg = a.distanceFromWallSum / a.distanceFromWallCount;
        double V = avg * avg * avg;
        a.surfaceToVolumeRatio = a.totalSurfaceArea / std::max(V, 1.0);
    }
    return a;
}

} // namespace spp
