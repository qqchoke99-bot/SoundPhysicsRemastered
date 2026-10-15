#pragma once
// Shared SPP probe scheduler: one full 200-ray probe per kTickRate ticks,
// results shared by reverb (room->FMOD) and occlusion (per-channel->FMOD).
// Single-threaded by design (no Java thread-pool explosion on mobile).
#include "spp/SppRays.hpp"
#include <map>

namespace spz::spp {

struct Engine {
    std::map<void*, Vec3> positions;
    std::map<void*, SppChannel> results;
    SppStats last;
    int tick = 0;
    bool hasProbe = false;

    void setPos(void* ch, const Vec3& pos);
    void prune(const std::map<void*, Vec3>& active);
    void clear();
    // Runs probeFull when due; otherwise returns cached stats.
    const SppStats& probe(const Vec3& eye, const Vec3& center);
    const SppChannel* channel(void* ch) const;
};

Engine& engine();

} // namespace spz::spp
