// Shared SPP probe scheduler.
#include "spp/SppEngine.hpp"
#include "spp/SppConfig.hpp"

namespace spz::spp {

Engine& engine() {
    static Engine e;
    return e;
}

void Engine::setPos(void* ch, const Vec3& pos) {
    if (ch) positions[ch] = pos;
}

void Engine::prune(const std::map<void*, Vec3>& active) {
    for (auto it = positions.begin(); it != positions.end();) {
        if (active.find(it->first) == active.end()) {
            results.erase(it->first);
            it = positions.erase(it);
        } else {
            ++it;
        }
    }
    for (auto it = results.begin(); it != results.end();) {
        if (active.find(it->first) == active.end())
            it = results.erase(it);
        else
            ++it;
    }
}

void Engine::clear() {
    positions.clear();
    results.clear();
    hasProbe = false;
    tick = 0;
}

const SppStats& Engine::probe(const Vec3& eye, const Vec3& center) {
    ++tick;
    bool due = !hasProbe || (tick % kTickRate == 0);
    if (due) {
        std::vector<std::pair<Vec3, SppChannel*>> sounds;
        sounds.reserve(positions.size());
        for (auto& kv : positions) {
            SppChannel& slot = results[kv.first]; // create slot
            sounds.emplace_back(kv.second, &slot);
        }
        last = probeFull(eye, center, sounds);
        hasProbe = true;
    }
    return last;
}

const SppChannel* Engine::channel(void* ch) const {
    auto it = results.find(ch);
    return it == results.end() ? nullptr : &it->second;
}

} // namespace spz::spp
