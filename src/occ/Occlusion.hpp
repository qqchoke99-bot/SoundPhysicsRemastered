#pragma once
#include "fmod/FmodApi.hpp"
namespace spz::occ {
void onSet3D(void* channel, const FMOD_VECTOR* pos);
void update();
void shutdown();
}
