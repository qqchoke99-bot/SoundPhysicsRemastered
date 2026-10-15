#include "mc/World.hpp"
#include <cmath>
#include "Log.hpp"

namespace spz::mc {
namespace {
PlayerState gPlayer;
void* gBlockSource = nullptr; // OPENCODE: filled from client dimension
}

bool initSignatures() {
    // OPENCODE TODO:
    // 1) Hook ClientInstance::update (VCX: hk_client_update) via pl::memory::resolveVtableFunction
    //    string "ClientInstance" / vtable index from reports for 1.26.52
    // 2) From client get LocalPlayer position + BlockSource*
    // 3) Resolve BlockSource::getBlock or IsSolidBlockingBlock
    //
    // Until filled: rayDistance returns maxDist (no occlusion geometry) but FMOD
    // reverb path still runs — same as "hook FMOD only" minimal mode.
    SPZ_LOGW("mc signatures not filled — reverb uses listener-only probe; OPENCODE: wire getBlock");
    return true;
}

PlayerState getPlayerState() {
    // OPENCODE: read from hooked client
    return gPlayer;
}

void setPlayerStateForTest(const PlayerState& s) { gPlayer = s; }

bool isSolid(int, int, int) {
    // OPENCODE: BlockSource getBlock != air
    return false;
}

float blockCost(int bx, int by, int bz) {
    // VCX caches by block id string "blockid '%s' cost=%.2f"
    return isSolid(bx, by, bz) ? 1.0f : 0.0f;
}

float rayDistance(const FMOD_VECTOR& origin, float dx, float dy, float dz, float maxDist, bool& hit) {
    hit = false;
    // DDA voxel walk — works once isSolid is real
    const float len = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (len < 1e-6f) return maxDist;
    dx /= len; dy /= len; dz /= len;
    const float step = 0.5f;
    float t = 0.0f;
    while (t < maxDist) {
        const float x = origin.x + dx * t;
        const float y = origin.y + dy * t;
        const float z = origin.z + dz * t;
        const int bx = (int)std::floor(x);
        const int by = (int)std::floor(y);
        const int bz = (int)std::floor(z);
        if (isSolid(bx, by, bz)) {
            hit = true;
            return t;
        }
        t += step;
    }
    return maxDist;
}

} // namespace spz::mc
