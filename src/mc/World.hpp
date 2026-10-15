#pragma once
#include "fmod/FmodApi.hpp"

namespace spz::mc {

// OPENCODE must resolve these from libminecraftpe.so for the target game build:
//   - ClientInstance* (or tick fn) for player position each frame
//   - BlockSource::getBlock / isSolidBlockingBlock
//   - optional: block name/id for absorption (blockCost in VCX)

struct PlayerState {
    FMOD_VECTOR pos{};
    FMOD_VECTOR vel{};
    bool valid = false;
};

bool initSignatures();           // resolve getBlock + client update
PlayerState getPlayerState();    // listener / player feet or eye
bool isSolid(int bx, int by, int bz);
float blockCost(int bx, int by, int bz);  // 0 air .. 1 solid absorber (VCX blockCost)

// Ray from origin along dir, maxDist blocks. Sets hit + distance.
// VCX: rayDistance(FMOD_VECTOR const&, float, float, float, float, bool&)
float rayDistance(const FMOD_VECTOR& origin, float dx, float dy, float dz, float maxDist, bool& hit);

}
