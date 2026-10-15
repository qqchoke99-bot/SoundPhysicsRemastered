#include "Log.hpp"
#include "fmod/FmodBridge.hpp"
#include "mc/World.hpp"
#include "spp/Version.hpp"

#include <pl/Mod.hpp>

namespace {

class SppMod {
public:
    static SppMod& instance() {
        static SppMod m;
        return m;
    }
    bool load(pl::mod::ModContext&) {
        SPP_LOGI("%s v%s — pure Sound Physics Perfected formulas (not SPR)", spp::version::Name.data(),
                 spp::version::Version.data());
        return true;
    }
    bool enable(pl::mod::ModContext&) {
        spp::fmodx::onEnable();
        spp::mc::installClientDriver();
        return true;
    }
    bool disable(pl::mod::ModContext&) {
        spp::mc::uninstallClientDriver();
        spp::fmodx::onDisable();
        return true;
    }
    bool unload(pl::mod::ModContext& c) { return disable(c); }
};

} // namespace

PL_REGISTER_MOD(SppMod, SppMod::instance())
