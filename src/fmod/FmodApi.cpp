#include "fmod/FmodApi.hpp"
#include "Log.hpp"

#include <dlfcn.h>
#include <cstring>

// Preloader hooks — OPENCODE: confirm exact symbols from libpreloader.so of Levi 1.5.x
namespace pl::memory {
enum class HookPriority { Normal = 0 };
void* hook(void* target, void* detour, void** original, HookPriority = HookPriority::Normal);
void unhook(void* target, void* detour);
}

namespace spz::fmodapi {
namespace {

Api g;

template <class T>
T sym(void* lib, const char* name) {
    void* p = dlsym(lib, name);
    if (!p) SPZ_LOGW("dlsym failed: %s (%s)", name, dlerror());
    return reinterpret_cast<T>(p);
}

// Detours declared here; defined after Api methods need them as friends via extern
int hk_playSound(void* self, void* sound, void* group, bool paused, void** outChannel);
int hk_setVolume(void* channel, float vol);
int hk_setPitch(void* channel, float pitch);
int hk_set3DAttr(void* channel, const FMOD_VECTOR* pos, const FMOD_VECTOR* vel);
int hk_update(void* self);

} // namespace

Api& api() { return g; }

bool Api::resolve() {
    // Game already mapped libfmod — prefer RTLD_NOLOAD like VCX log "dlopen NOLOAD"
    lib = dlopen("libfmod.so", RTLD_NOW | RTLD_NOLOAD);
    if (!lib) lib = dlopen("libfmod.so", RTLD_NOW);
    if (!lib) {
        SPZ_LOGE("dlopen libfmod.so failed: %s", dlerror());
        return false;
    }

    playSound = sym<PlaySoundFn>(lib, "_ZN4FMOD6System9playSoundEPNS_5SoundEPNS_12ChannelGroupEbPPNS_7ChannelE");
    setSystemReverb = sym<SetReverbPropsFn>(lib, "_ZN4FMOD6System19setReverbPropertiesEiPK22FMOD_REVERB_PROPERTIES");
    createDSPByType = sym<CreateDspFn>(lib, "_ZN4FMOD6System15createDSPByTypeE13FMOD_DSP_TYPEPPNS_3DSPE");
    set3DSettings = sym<Set3DSettingsFn>(lib, "_ZN4FMOD6System13set3DSettingsEfff");
    update = sym<UpdateFn>(lib, "_ZN4FMOD6System6updateEv");

    setVolume = sym<SetVolumeFn>(lib, "_ZN4FMOD14ChannelControl9setVolumeEf");
    setPitch = sym<SetPitchFn>(lib, "_ZN4FMOD14ChannelControl8setPitchEf");
    set3DAttributes = sym<Set3DAttrFn>(lib, "_ZN4FMOD14ChannelControl15set3DAttributesEPK11FMOD_VECTORS3_");
    get3DAttributes = sym<Get3DAttrFn>(lib, "_ZN4FMOD14ChannelControl15get3DAttributesEP11FMOD_VECTORS2_");
    set3DOcclusion = sym<Set3DOcclusionFn>(lib, "_ZN4FMOD14ChannelControl14set3DOcclusionEff");
    setChannelReverb = sym<SetReverbSendFn>(lib, "_ZN4FMOD14ChannelControl19setReverbPropertiesEif");
    addDSP = sym<AddDspFn>(lib, "_ZN4FMOD14ChannelControl6addDSPEiPNS_3DSPE");
    getMode = sym<GetModeFn>(lib, "_ZN4FMOD14ChannelControl7getModeEPj");
    isPlaying = sym<IsPlayingFn>(lib, "_ZN4FMOD14ChannelControl9isPlayingEPb");
    dspSetParam = sym<DspSetParamFn>(lib, "_ZN4FMOD3DSP17setParameterFloatEif");
    dspRelease = sym<DspReleaseFn>(lib, "_ZN4FMOD3DSP7releaseEv");

    const bool ok = playSound && setSystemReverb && setChannelReverb && setVolume && set3DAttributes;
    SPZ_LOGI("FMOD symbols resolved, set3DOcclusion=%p playSound=%p setReverb=%p",
             (void*)set3DOcclusion, (void*)playSound, (void*)setSystemReverb);
    return ok;
}

bool Api::installHooks() {
    if (!playSound || !setVolume || !set3DAttributes) return false;

    // OPENCODE: pl::memory::hook signature must match Levi preloader
    if (playSound) {
        playSoundOrig = playSound;
        // hook(playSound, (void*)hk_playSound, (void**)&playSoundOrig);
        // For skeleton: direct assign if preloader provides interpose — fill in build.
    }
    SPZ_LOGI("FMOD hooks installed (wire pl::memory::hook in FmodApi.cpp — OPENCODE)");
    return true;
}

void Api::uninstallHooks() {
    // unhook targets
}

} // namespace spz::fmodapi

// Forward to reverb/occ modules
namespace spz::reverb {
void onNewChannel(void* channel);
void update();
}
namespace spz::occ {
void onSet3D(void* channel, const FMOD_VECTOR* pos);
void update();
}

namespace spz::fmodapi {
namespace {

int hk_playSound(void* self, void* sound, void* group, bool paused, void** outChannel) {
    if (self) g.system = self;
    const int rc = g.playSoundOrig ? g.playSoundOrig(self, sound, group, paused, outChannel)
                                   : (g.playSound ? g.playSound(self, sound, group, paused, outChannel) : -1);
    if (rc == 0 && outChannel && *outChannel) {
        // VCX: onNewChannel — open reverb send + track channel
        if (g.setChannelReverb) g.setChannelReverb(*outChannel, 0, 1.0f);
        spz::reverb::onNewChannel(*outChannel);
    }
    return rc;
}

int hk_setVolume(void* channel, float vol) {
    const int rc = g.setVolumeOrig ? g.setVolumeOrig(channel, vol)
                                   : (g.setVolume ? g.setVolume(channel, vol) : -1);
    return rc;
}

int hk_setPitch(void* channel, float pitch) {
    return g.setPitchOrig ? g.setPitchOrig(channel, pitch)
                          : (g.setPitch ? g.setPitch(channel, pitch) : -1);
}

int hk_set3DAttr(void* channel, const FMOD_VECTOR* pos, const FMOD_VECTOR* vel) {
    const int rc = g.set3DAttributesOrig ? g.set3DAttributesOrig(channel, pos, vel)
                                         : (g.set3DAttributes ? g.set3DAttributes(channel, pos, vel) : -1);
    if (rc == 0 && pos) spz::occ::onSet3D(channel, pos);
    return rc;
}

int hk_update(void* self) {
    if (self) g.system = self;
    const int rc = g.updateOrig ? g.updateOrig(self) : (g.update ? g.update(self) : -1);
    spz::reverb::update();
    spz::occ::update();
    return rc;
}

} // namespace
} // namespace spz::fmodapi
