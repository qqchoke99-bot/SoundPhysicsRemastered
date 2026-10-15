#include "occ/Occlusion.hpp"
#include "mc/World.hpp"
#include "cfg/Config.hpp"
#include "Log.hpp"
#include <cmath>
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
}

void update() {
    if (!cfg::occlusionEnabled()) return;
    auto& a = fmodapi::api();
    if (!a.set3DOcclusion) return;
    auto player = mc::getPlayerState();
    if (!player.valid) return;

    FMOD_VECTOR ear = player.pos;
    ear.y += 1.62f;

    std::lock_guard lock(gMu);
    for (auto& kv : gMap) {
        void* ch = kv.first;
        auto& st = kv.second;
        float dx = st.pos.x - ear.x, dy = st.pos.y - ear.y, dz = st.pos.z - ear.z;
        float dist = std::sqrt(dx*dx+dy*dy+dz*dz);
        if (dist < 0.5f) {
            a.set3DOcclusion(ch, 0.f, 0.f);
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
}
} // namespace spz::occ
