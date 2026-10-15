#include "fmod/FmodBridge.hpp"
#include "Log.hpp"
#include "spp/Config.hpp"
#include "spp/Probe.hpp"
#include "mc/World.hpp"

#include <pl/memory/Hook.hpp>
#include <dlfcn.h>
#include <chrono>
#include <cmath>

namespace spp::fmodx {
namespace {

struct ReverbProps {
    float DecayTime, EarlyDelay, LateDelay, HFReference, HFDecayRatio;
    float Diffusion, Density, LowShelfFrequency, LowShelfGain, HighCut, EarlyLateMix, WetLevel;
};

using PlaySoundFn = int (*)(void*, void*, void*, bool, void**);
using SetRevFn = int (*)(void*, int, const ReverbProps*);
using SetSendFn = int (*)(void*, int, float);
using UpdateFn = int (*)(void*);
using SetListenerFn = int (*)(void*, int, const float*, const float*, const float*, const float*);

void* gLib = nullptr;
void* gSys = nullptr;
PlaySoundFn gPlay = nullptr, gPlayO = nullptr;
SetRevFn gSetRev = nullptr;
SetSendFn gSetSend = nullptr;
UpdateFn gUpdate = nullptr, gUpdateO = nullptr;
SetListenerFn gList = nullptr, gListO = nullptr;

float gEar[3] = {0, 0, 0};
bool gHaveEar = false;

int hk_play(void* self, void* s, void* g, bool p, void** out) {
    if (self) gSys = self;
    int rc = gPlayO ? gPlayO(self, s, g, p, out) : -1;
    if (rc == 0 && out && *out && gSetSend) gSetSend(*out, 0, 1.0f);
    return rc;
}

int hk_list(void* self, int id, const float* pos, const float* vel, const float* f, const float* u) {
    if (self) gSys = self;
    int rc = gListO ? gListO(self, id, pos, vel, f, u) : -1;
    if (rc == 0 && pos) {
        gEar[0] = pos[0];
        gEar[1] = pos[1];
        gEar[2] = pos[2];
        gHaveEar = true;
        mc::setListener(pos[0], pos[1], pos[2]);
    }
    return rc;
}

void tickRoom() {
    if (!cfg::settings().reverb || !gSys || !gSetRev || !gHaveEar) return;
    spp::Vec3 o{gEar[0], gEar[1] + 0.0f, gEar[2]};
    auto accum = probeListener(o, 32.f, 16, mc::sampleBlock);
    auto enh = calculateEnhancedReverb(accum);
    auto efx = toEfx(enh, cfg::settings());
    applyEfx(efx);
    SPP_LOGI("SPP profile=%s indoors=%d rt60=%.2fs gain=%.2f early=%.0fms",
             enh.acousticProfile.c_str(), (int)enh.isIndoors, efx.decayTimeSec, efx.gain,
             efx.reflectionsDelaySec * 1000.f);
}

int hk_update(void* self) {
    if (self) gSys = self;
    int rc = gUpdateO ? gUpdateO(self) : -1;
    static auto last = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last).count() >= 100) {
        last = now;
        tickRoom();
    }
    return rc;
}

bool hookOne(const char* n, void* t, void* d, void** o) {
    if (!t) {
        SPP_LOGE("%s missing", n);
        return false;
    }
    int rc = pl::memory::hook(t, d, o, pl::memory::HookPriority::Normal);
    if (rc != 0) {
        SPP_LOGE("%s hook failed %d", n, rc);
        return false;
    }
    SPP_LOGI("%s hooked", n);
    return true;
}

} // namespace

bool resolve() {
    gLib = dlopen("libfmod.so", RTLD_NOW | RTLD_NOLOAD);
    if (!gLib) gLib = dlopen("libfmod.so", RTLD_NOW);
    if (!gLib) return false;
    auto S = [&](const char* n) { return dlsym(gLib, n); };
    gPlay = (PlaySoundFn)S("_ZN4FMOD6System9playSoundEPNS_5SoundEPNS_12ChannelGroupEbPPNS_7ChannelE");
    gSetRev = (SetRevFn)S("_ZN4FMOD6System19setReverbPropertiesEiPK22FMOD_REVERB_PROPERTIES");
    gSetSend = (SetSendFn)S("_ZN4FMOD14ChannelControl19setReverbPropertiesEif");
    gUpdate = (UpdateFn)S("_ZN4FMOD6System6updateEv");
    gList = (SetListenerFn)S("_ZN4FMOD6System23set3DListenerAttributesEiPK11FMOD_VECTORS3_S4_S4_");
    return gPlay && gSetRev;
}

bool installHooks() {
    bool ok = true;
    ok &= hookOne("playSound", (void*)gPlay, (void*)&hk_play, (void**)&gPlayO);
    ok &= hookOne("update", (void*)gUpdate, (void*)&hk_update, (void**)&gUpdateO);
    if (gList) hookOne("listener", (void*)gList, (void*)&hk_list, (void**)&gListO);
    return ok;
}

void uninstallHooks() {
    if (gPlay) pl::memory::unhook((void*)gPlay, (void*)&hk_play);
    if (gUpdate) pl::memory::unhook((void*)gUpdate, (void*)&hk_update);
    if (gList) pl::memory::unhook((void*)gList, (void*)&hk_list);
}

void applyEfx(const EfxParams& p) {
    if (!gSys || !gSetRev) return;
    ReverbProps r{};
    r.DecayTime = p.decayTimeSec * 1000.f; // FMOD ms
    r.EarlyDelay = p.reflectionsDelaySec * 1000.f;
    r.LateDelay = p.lateReverbDelaySec * 1000.f;
    r.HFReference = 5000.f;
    r.HFDecayRatio = p.gainHF * 1.2f;
    r.Diffusion = p.diffusion * 100.f;
    r.Density = p.density * 100.f;
    r.LowShelfFrequency = 250.f;
    r.LowShelfGain = 0.f;
    r.HighCut = 2000.f + p.gainHF * 6000.f;
    r.EarlyLateMix = 50.f;
    // OpenAL gain 0..1 → FMOD wet dB (SPP intensity)
    float wetDb = -80.f;
    if (p.gain > 0.001f) {
        wetDb = 20.f * std::log10(std::max(0.001f, p.gain)) + 6.f; // boost into audible
        wetDb = std::max(-36.f, std::min(cfg::settings().maxWetDb, wetDb));
    }
    r.WetLevel = wetDb;
    int rc = gSetRev(gSys, 0, &r);
    SPP_LOGI("FMOD reverb rc=%d decay=%.0fms wet=%.1fdB (from SPP EFX)", rc, r.DecayTime, wetDb);
}

void onEnable() {
    if (!resolve()) {
        SPP_LOGE("FMOD resolve failed");
        return;
    }
    installHooks();
}
void onDisable() { uninstallHooks(); }

} // namespace spp::fmodx
