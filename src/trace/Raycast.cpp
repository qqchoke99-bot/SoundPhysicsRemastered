#include "trace/Raycast.hpp"
#include "mc/World.hpp"
#include <algorithm>
#include <cmath>

namespace spz::trace {
namespace {

RayHit march(const FMOD_VECTOR& o, float dx, float dy, float dz, float maxDist) {
    RayHit h;
    float len = std::sqrt(dx*dx+dy*dy+dz*dz);
    if (len < 1e-6f) return h;
    dx/=len; dy/=len; dz/=len;
    const float step = 0.25f;
    float t = step;
    int pbx=0,pby=0,pbz=0;
    bool have=false;
    while (t <= maxDist) {
        int bx = (int)std::floor(o.x + dx*t);
        int by = (int)std::floor(o.y + dy*t);
        int bz = (int)std::floor(o.z + dz*t);
        if (mc::isSolid(bx,by,bz)) {
            h.hit = true;
            h.distance = t;
            h.bx=bx; h.by=by; h.bz=bz;
            h.point = {o.x+dx*t, o.y+dy*t, o.z+dz*t};
            // Face normal from previous cell
            if (have) {
                if (bx!=pbx) h.normal = {bx>pbx?-1.f:1.f, 0, 0};
                else if (by!=pby) h.normal = {0, by>pby?-1.f:1.f, 0};
                else h.normal = {0, 0, bz>pbz?-1.f:1.f};
            }
            return h;
        }
        pbx=bx; pby=by; pbz=bz; have=true;
        t += step;
    }
    return h;
}

void reflect(float& dx, float& dy, float& dz, const FMOD_VECTOR& n) {
    // d' = d - 2(d·n)n
    float dot = dx*n.x + dy*n.y + dz*n.z;
    dx -= 2.f*dot*n.x;
    dy -= 2.f*dot*n.y;
    dz -= 2.f*dot*n.z;
}

} // namespace

void traceSpecular(const FMOD_VECTOR& origin, const FMOD_VECTOR& listener,
                   const RaycastConfig& cfg, std::vector<SpecularArrival>& out) {
    out.clear();
    // Fibonacci-ish directions on sphere
    const int N = std::max(4, std::min(32, cfg.rays));
    for (int i = 0; i < N; ++i) {
        float z = 1.f - 2.f * (i + 0.5f) / N;
        float r = std::sqrt(std::max(0.f, 1.f - z*z));
        float th = 2.399963f * i; // golden angle
        float dx = r * std::cos(th), dy = z, dz = r * std::sin(th);

        FMOD_VECTOR pos = origin;
        float path = 0;
        float energy = 1.f;
        float rdx=dx, rdy=dy, rdz=dz;

        for (int bounce = 0; bounce <= cfg.maxBounces; ++bounce) {
            RayHit hit = march(pos, rdx, rdy, rdz, cfg.maxDistance - path);
            if (!hit.hit) break;
            path += hit.distance;
            energy *= cfg.reflectivity;
            energy *= std::exp(-cfg.airAbsorption * hit.distance);
            if (energy < 0.02f) break;

            // Path origin -> hit -> listener (approx last leg)
            float lx = listener.x - hit.point.x;
            float ly = listener.y - hit.point.y;
            float lz = listener.z - hit.point.z;
            float leg = std::sqrt(lx*lx+ly*ly+lz*lz);
            float total = path + leg;
            SpecularArrival a;
            a.order = bounce + 1;
            a.pathLen = total;
            a.delayMs = 1000.f * total / std::max(1.f, cfg.speedOfSound);
            a.gain = energy / (1.f + 0.08f * total);
            out.push_back(a);

            // Reflect for next bounce
            reflect(rdx, rdy, rdz, hit.normal);
            pos = {hit.point.x + rdx*0.05f, hit.point.y + rdy*0.05f, hit.point.z + rdz*0.05f};
            if (path >= cfg.maxDistance) break;
        }
    }
}

EnclosureProbe probeEnclosure(const FMOD_VECTOR& origin, float radius, int dirs) {
    EnclosureProbe p;
    const int N = std::max(6, dirs);
    int hits = 0;
    float sum = 0;
    p.sky = 0;
    for (int i = 0; i < N; ++i) {
        float z = 1.f - 2.f * (i + 0.5f) / N;
        float r = std::sqrt(std::max(0.f, 1.f - z*z));
        float th = 2.399963f * i;
        float dx = r*std::cos(th), dy = z, dz = r*std::sin(th);
        RayHit h = march(origin, dx, dy, dz, radius);
        if (h.hit) {
            ++hits;
            sum += h.distance;
            if (dy > 0.85f) p.ceilDist = h.distance;
            if (dy < -0.85f) p.floorDist = h.distance;
        } else if (dy > 0.85f) {
            p.sky = 1;
        }
    }
    p.enclosure = hits / (float)N;
    p.meanDist = hits ? sum / hits : radius * 0.5f;
    return p;
}

} // namespace spz::trace
