#pragma once
#include "spp/EnhancedReverb.hpp"

namespace spp {
struct Vec3 { float x, y, z; };
// Callback: solid + absorption at block. absorption 0.05 default stone-like
using SampleFn = void (*)(int bx, int by, int bz, bool& solid, float& absorption);

SurfaceAccum probeListener(const Vec3& origin, float maxDist, int rays, SampleFn sample);
}
