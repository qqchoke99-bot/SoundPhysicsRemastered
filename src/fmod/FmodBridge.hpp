#pragma once
#include "spp/EnhancedReverb.hpp"

namespace spp::fmodx {
bool resolve();
bool installHooks();
void uninstallHooks();
void applyEfx(const EfxParams& p); // System setReverbProperties
void onEnable();
void onDisable();
}
