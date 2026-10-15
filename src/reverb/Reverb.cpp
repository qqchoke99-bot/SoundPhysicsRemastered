#include "reverb/Reverb.hpp"
#include "fmod/FmodApi.hpp"
#include "mc/World.hpp"
#include "cfg/Config.hpp"
#include "trace/PathTrace.hpp"
#include "Log.hpp"

#include <cmath>
#include <mutex>
#include <vector>

namespace spz::reverb {
namespace {

std::mutex gMu;
std::vector<void*> gChannels;

void sendProps(float decayMs, float earlyMs, float wetDb, float diffusion, float density) {
    auto& a = fmodapi::api();
    if (!a.system || !a.setSystemReverb) return;
    FMOD_REVERB_PROPERTIES p{};
    p.DecayTime = decayMs;
    p.EarlyDelay = earlyMs;
    p.LateDelay = earlyMs * 1.5f + 5.f;
    p.HFReference = 5000.f;
    p.HFDecayRatio = 0.65f;
    p.Diffusion = diffusion * 100.f;
    p.Density = density * 100.f;
    p.LowShelfFrequency = 250.f;
    p.LowShelfGain = 0.f;
    p.HighCut = 7500.f;
    p.EarlyLateMix = 55.f;
    p.WetLevel = wetDb;
    int rc = a.setSystemReverb(a.system, 0, &p);
    SPZ_LOGI("reverb send rc=%d decay=%.0f wet=%.1f (RT+PT hybrid)", rc, decayMs, wetDb);
}

} // namespace

void onNewChannel(void* channel) {
    if (!channel) return;
    auto& a = fmodapi::api();
    if (a.setChannelReverb) a.setChannelReverb(channel, 0, 1.0f);
    // Optional: attach Echo DSP for specular delay — OPENCODE if createDSP path desired
    std::lock_guard lock(gMu);
    gChannels.push_back(channel);
}

void update() {
    if (!cfg::reverbEnabled()) return;
    auto ps = mc::getPlayerState();
    FMOD_VECTOR eye = ps.pos;
    eye.y += 1.62f;

    trace::RaycastConfig ray{};
    ray.rays = 12;
    ray.maxBounces = 2;
    ray.maxDistance = 28.f;
    ray.speedOfSound = 343.f;
    ray.reflectivity = 0.88f;

    trace::PathTraceConfig path{};
    path.paths = 12;
    path.scatterDepth = 3;
    path.maxSegment = 22.f;
    path.speedOfSound = 343.f;
    path.reflectivity = 0.85f;

    auto h = trace::hybridRoom(eye, ray, path, cfg::maxRt60(), cfg::maxWetDb());

    SPZ_LOGI("hybrid e=%.2f mean=%.1f sky=%d wet=%.1f decay=%.0f echo=%.0fms diff=%.2f",
             h.room.enclosure, h.room.meanDist, h.room.sky, h.wetDb, h.decayMs,
             h.echoDelayMs, h.diffusion);

    sendProps(h.decayMs, h.earlyMs, h.wetDb, h.diffusion, h.density);

    auto& a = fmodapi::api();
    std::lock_guard lock(gMu);
    for (void* ch : gChannels) {
        if (!ch) continue;
        if (a.setChannelReverb) a.setChannelReverb(ch, 0, 1.0f);
    }
}

void shutdown() {
    std::lock_guard lock(gMu);
    gChannels.clear();
}

} // namespace spz::reverb
