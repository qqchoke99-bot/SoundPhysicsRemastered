#include "mc/World.hpp"
#include "Log.hpp"

#include <pl/memory/Hook.hpp>

#include <cstdint>
#include <string_view>

namespace pl::memory {
uintptr_t resolveVtableFunction(std::string_view name, std::size_t index, std::string_view module);
}

namespace spp::mc {
namespace {
void* gRegion = nullptr;
void* gAir = nullptr;
bool gHaveAir = false;

void* vt(void* o, size_t i) {
    if (!o) return nullptr;
    void** v = *reinterpret_cast<void***>(o);
    return v ? v[i] : nullptr;
}

using UpdateFn = void (*)(void*, bool);
UpdateFn gUpO = nullptr;
void* gUpT = nullptr;

void hk(void* self, bool b) {
    if (gUpO) gUpO(self, b);
    using GR = void* (*)(void*);
    auto* fn = reinterpret_cast<GR>(vt(self, 31));
    if (fn) setRegion(fn(self));
}
} // namespace

void setListener(float, float, float) {}

void setRegion(void* r) {
    gRegion = r;
    gHaveAir = false;
    gAir = nullptr;
}

void sampleBlock(int bx, int by, int bz, bool& solid, float& absorption) {
    solid = false;
    absorption = 0.05f;
    if (!gRegion) return;
    struct BP {
        int x, y, z;
    } p{bx, by, bz};
    using GB = void* (*)(void*, const BP&);
    auto* fn = reinterpret_cast<GB>(vt(gRegion, 2));
    if (!fn) return;
    void* blk = fn(gRegion, p);
    if (!blk) return;
    if (!gHaveAir) {
        BP airp{bx, 320, bz};
        gAir = fn(gRegion, airp);
        gHaveAir = true;
    }
    if (gAir && blk == gAir) {
        solid = false;
        absorption = 0.01f;
        return;
    }
    solid = true;
    absorption = 0.12f;
}

bool installClientDriver() {
    uintptr_t a = pl::memory::resolveVtableFunction("14ClientInstance", 25, "libminecraftpe.so");
    if (!a) {
        SPP_LOGW("no ClientInstance driver");
        return false;
    }
    gUpT = (void*)a;
    if (pl::memory::hook(gUpT, (void*)&hk, (void**)&gUpO, pl::memory::HookPriority::Normal) != 0) {
        SPP_LOGE("client hook failed");
        return false;
    }
    SPP_LOGI("ClientInstance driver on");
    return true;
}

void uninstallClientDriver() {
    if (gUpT) pl::memory::unhook(gUpT, (void*)&hk);
    gUpT = nullptr;
}

} // namespace spp::mc
