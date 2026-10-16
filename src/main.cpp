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
        SPP_LOGI("%s v%s — SPP formulas + BlockSource isSolidBlockingBlock (1.26.52)",
                 spp::version::Name.data(), spp::version::Version.data());
        return true;
    }

    bool enable(pl::mod::ModContext&) {
        // World first so probe can use BlockSource once FMOD listener ticks
        spp::mc::installWorldAccess();
        spp::fmodx::onEnable();
        return true;
    }

    bool disable(pl::mod::ModContext&) {
        spp::fmodx::onDisable();
        spp::mc::uninstallWorldAccess();
        return true;
    }

    bool unload(pl::mod::ModContext& c) { return disable(c); }
};

} // namespace

PL_REGISTER_MOD(SppMod, SppMod::instance())
