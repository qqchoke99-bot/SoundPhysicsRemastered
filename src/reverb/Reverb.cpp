#include "reverb/Reverb.hpp"
#include "fmod/FmodApi.hpp"
#include "mc/World.hpp"
#include "cfg/Config.hpp"
#include "spp/SppEngine.hpp"
#include "spp/SppReverb.hpp"
#include "spp/SppConfig.hpp"
#include "Log.hpp"

#include <cmath>
#include <mutex>
#include <vector>

namespace spz::reverb {
namespace {

std::mutex gMu;
std::vector<void*> gChannels;

void sendProps(const spp::SppFmod& f) {
    auto& a = fmodapi::api();
    if (!a.system || !a.setSystemReverb) return;
    FMOD_REVERB_PROPERTIES p{};
    p.DecayTime = f.decayMs;
    p.EarlyDelay = f.earlyMs;
    p.LateDelay = f.lateMs;
    p.HFReference = f.hfRef;
    p.HFDecayRatio = f.hfRatio;
    p.Diffusion = f.diffusion * 100.f;
    p.Density = f.density * 100.f;
    p.LowShelfFrequency = 250.f;
    p.LowShelfGain = 0.f;
    p.HighCut = 7500.f;
    p.EarlyLateMix = 55.f;
    p.WetLevel = f.wetDb;
    int rc = a.setSystemReverb(a.system, 0, &p);
    SPZ_LOGI("spp reverb rc=%d decay=%.0f wet=%.1f encl=%.2f indoors=%d echo=%.0fms",
             rc, f.decayMs, f.wetDb, f.enclosure, (int)f.indoors, f.echoDelayMs);
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
    if (!ps.valid) return;
    spp::Vec3 eye{ps.pos.x, ps.pos.y + 1.62, ps.pos.z};
    spp::Vec3 center{ps.pos.x, ps.pos.y + 0.9, ps.pos.z};

    // One shared full-accuracy probe (200 rays, 3 bounces) per kTickRate.
    const spp::SppStats& st = spp::engine().probe(eye, center);
    spp::SppFmod f = spp::computeReverb(st);
    sendProps(f);

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
