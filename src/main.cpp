#include "Log.hpp"
#include "cfg/Config.hpp"
#include "fmod/FmodApi.hpp"
#include "mc/World.hpp"
#include "reverb/Reverb.hpp"
#include "occ/Occlusion.hpp"
#include "spz/Version.hpp"

#include <cstring>

// Levi / preloader mod registration — OPENCODE: match exact ABI of Levi 1.5.x
extern "C" {

struct PLModInfo {
    const char* name;
    const char* version;
    const char* author;
    const char* description;
};

// Placeholder: real Levi uses PLGetModRegistration returning vtable/callbacks.
__attribute__((visibility("default"))) void* PLGetModRegistration() {
    SPZ_LOGI("loading %s v%s (target MC %s)",
             spz::version::Name.data(), spz::version::Version.data(),
             spz::version::TargetMinecraftVersion.data());
    spz::cfg::loadDefaults();
    if (!spz::fmodapi::api().resolve()) {
        SPZ_LOGE("FMOD resolve failed");
        return nullptr;
    }
    spz::mc::initSignatures();
    spz::fmodapi::api().installHooks();
    SPZ_LOGI("SoundPhysicz recreate ready (hook FMOD + getBlock when wired)");
    return (void*)1; // OPENCODE: return real registration object
}

__attribute__((visibility("default"))) void PLModUnload() {
    spz::reverb::shutdown();
    spz::occ::shutdown();
    spz::fmodapi::api().uninstallHooks();
}

} // extern C
