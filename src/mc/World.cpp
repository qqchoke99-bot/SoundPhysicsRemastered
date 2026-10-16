#include "mc/World.hpp"
#include "Log.hpp"

#include <pl/memory/Hook.hpp>
#include <pl/memory/Signature.hpp>

#include <cstdint>
#include <string_view>

namespace pl::memory {
uintptr_t resolveVtableFunction(std::string_view name, std::size_t index, std::string_view module);
}

namespace spp::mc {
namespace {

void* gBlockSource = nullptr;

// bool BlockSource::isSolidBlockingBlock(BlockPos const&)
// Pattern unique on 1.26.52: ?? ?? ?? F9 ?? ?? ?? B9 ?? ?? ?? 29 ?? ?? ?? F9 E1 03 08 2A
using IsSolidFn = bool (*)(void* self, const BlockPosRaw* pos);
IsSolidFn gIsSolid = nullptr;

using ClientUpdateFn = void (*)(void* self, bool);
ClientUpdateFn gClientUpdateOrig = nullptr;
void* gClientUpdateTarget = nullptr;

constexpr std::size_t kClientUpdateSlot = 25;
constexpr std::size_t kClientRegionSlot = 31;

void* vtableFn(void* obj, std::size_t index) {
    if (!obj) return nullptr;
    void** vt = *reinterpret_cast<void***>(obj);
    return vt ? vt[index] : nullptr;
}

void onClientUpdate(void* self, bool arg) {
    if (gClientUpdateOrig) gClientUpdateOrig(self, arg);
    using GetRegionFn = void* (*)(void*);
    auto* getRegion = reinterpret_cast<GetRegionFn>(vtableFn(self, kClientRegionSlot));
    if (getRegion) {
        if (void* region = getRegion(self)) setRegion(region);
    }
}

bool resolveIsSolidBlocking() {
    // Resound-style signature scan on libminecraftpe.so
    const uintptr_t addr = pl::memory::resolveSignature(
        "?? ?? ?? F9 ?? ?? ?? B9 ?? ?? ?? 29 ?? ?? ?? F9 E1 03 08 2A", "libminecraftpe.so");
    if (!addr) {
        SPP_LOGW("isSolidBlockingBlock: pattern not found (need 1.26.52 libminecraftpe)");
        return false;
    }
    gIsSolid = reinterpret_cast<IsSolidFn>(addr);
    SPP_LOGI("BlockSource::isSolidBlockingBlock @ %p", (void*)gIsSolid);
    return true;
}

bool installClientDriver() {
    const uintptr_t addr =
        pl::memory::resolveVtableFunction("14ClientInstance", kClientUpdateSlot, "libminecraftpe.so");
    if (!addr) {
        SPP_LOGW("14ClientInstance slot %zu failed — BlockSource will stay null until fixed",
                 kClientUpdateSlot);
        return false;
    }
    gClientUpdateTarget = reinterpret_cast<void*>(addr);
    const int rc = pl::memory::hook(gClientUpdateTarget, reinterpret_cast<void*>(&onClientUpdate),
                                    reinterpret_cast<void**>(&gClientUpdateOrig),
                                    pl::memory::HookPriority::Normal);
    if (rc != 0) {
        SPP_LOGE("ClientInstance update hook failed rc=%d", rc);
        gClientUpdateTarget = nullptr;
        return false;
    }
    SPP_LOGI("ClientInstance update hooked — feeding BlockSource* each tick");
    return true;
}

} // namespace

void setListener(float, float, float) {}

void setRegion(void* blockSource) { gBlockSource = blockSource; }

bool isSolidBlocking(void* blockSource, int x, int y, int z) {
    if (!blockSource || !gIsSolid) return false;
    BlockPosRaw pos{x, y, z};
    return gIsSolid(blockSource, &pos);
}

void sampleBlock(int bx, int by, int bz, bool& solid, float& absorption) {
    solid = false;
    absorption = 0.05f;
    if (!gBlockSource || !gIsSolid) return;
    solid = isSolidBlocking(gBlockSource, bx, by, bz);
    absorption = solid ? 0.12f : 0.01f;
}

bool installWorldAccess() {
    const bool solidOk = resolveIsSolidBlocking();
    const bool clientOk = installClientDriver();
    SPP_LOGI("world: isSolidBlocking=%d clientDriver=%d", (int)solidOk, (int)clientOk);
    return solidOk || clientOk;
}

void uninstallWorldAccess() {
    if (gClientUpdateTarget)
        pl::memory::unhook(gClientUpdateTarget, reinterpret_cast<void*>(&onClientUpdate));
    gClientUpdateTarget = nullptr;
    gClientUpdateOrig = nullptr;
    gIsSolid = nullptr;
    gBlockSource = nullptr;
}

} // namespace spp::mc
