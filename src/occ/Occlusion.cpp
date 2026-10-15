#include "occ/Occlusion.hpp"
#include "mc/World.hpp"
#include "cfg/Config.hpp"
#include "spp/SppEngine.hpp"
#include "spp/SppConfig.hpp"
#include "Log.hpp"
#include <cmath>
#include <map>
#include <mutex>
#include <unordered_map>

namespace spz::occ {
namespace {
struct State { FMOD_VECTOR pos{}; float occ = 0; };
std::mutex gMu;
std::unordered_map<void*, State> gMap;
}

void onSet3D(void* channel, const FMOD_VECTOR* pos) {
    if (!channel || !pos || !cfg::occlusionEnabled()) return;
    std::lock_guard lock(gMu);
    gMap[channel].pos = *pos;
    spp::engine().setPos(channel, {pos->x, pos->y, pos->z});
}

void update() {
    if (!cfg::occlusionEnabled()) return;
    auto& a = fmodapi::api();
    if (!a.set3DOcclusion) return;
    auto player = mc::getPlayerState();
    if (!player.valid) return;

    FMOD_VECTOR ear = player.pos;
    ear.y += 1.62f;
    spp::Vec3 eye{ear.x, ear.y, ear.z};
    spp::Vec3 center{player.pos.x, player.pos.y + 0.9f, player.pos.z};

    std::lock_guard lock(gMu);
    // Prune dead channels from the shared engine.
    std::map<void*, spp::Vec3> active;
    for (auto& kv : gMap) active[kv.first] = {kv.second.pos.x, kv.second.pos.y, kv.second.pos.z};
    spp::engine().prune(active);
    // Shared probe (cached unless due) so per-channel SPP data is fresh.
    spp::engine().probe(eye, center);

    for (auto& kv : gMap) {
        void* ch = kv.first;
        auto& st = kv.second;
        float dx = st.pos.x - ear.x, dy = st.pos.y - ear.y, dz = st.pos.z - ear.z;
        float dist = std::sqrt(dx*dx+dy*dy+dz*dz);
        if (dist < 0.5f) {
            a.set3DOcclusion(ch, 0.f, 0.f);
            continue;
        }
        if (dist > spp::kProcRange) continue; // SPP procRange: out of scope
        // Full-accuracy path: permeation-weighted occlusion from the probe.
        if (const spp::SppChannel* sc = spp::engine().channel(ch)) {
            // shortcutDirectionality: clear permeation (~1) => fully direct.
            float direct = sc->permeation >= 0.99f ? 0.0f : (float)sc->occlusion;
            a.set3DOcclusion(ch, direct, direct * 0.7f);
            continue;
        }
        bool hit = false;
        float maxR = cfg::occlusionRange();
        float blocked = 0.f;
        // March and accumulate blockCost (VCX style)
        const int steps = std::min(32, (int)(dist * 2));
        for (int i = 1; i <= steps; ++i) {
            float t = (dist * i) / steps;
            int bx = (int)std::floor(ear.x + dx / dist * t);
            int by = (int)std::floor(ear.y + dy / dist * t);
            int bz = (int)std::floor(ear.z + dz / dist * t);
            blocked += mc::blockCost(bx, by, bz);
        }
        float direct = std::min(1.f, blocked * cfg::occlusionDbPerBlock() / 10.f);
        float reverbOcc = direct * 0.7f;
        a.set3DOcclusion(ch, direct, reverbOcc);
        // optional volume cut / lowpass via setVolume / DSP — OPENCODE if needed
    }
}

void shutdown() {
    std::lock_guard lock(gMu);
    gMap.clear();
    spp::engine().clear();
}
} // namespace spz::occ
