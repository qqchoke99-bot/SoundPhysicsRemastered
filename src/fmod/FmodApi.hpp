#pragma once
// Resolved at runtime from the game's already-loaded libfmod.so — same as VCX.
// No FMOD SDK headers required.

struct FMOD_VECTOR { float x, y, z; };

// Minimal FMOD_REVERB_PROPERTIES (12 floats) — verified layout may differ per build;
// we write the same fields SoundPhysicz logs: decay (ms), early (ms), wet (dB).
struct FMOD_REVERB_PROPERTIES {
    float DecayTime;          // ms
    float EarlyDelay;         // ms
    float LateDelay;          // ms
    float HFReference;        // Hz
    float HFDecayRatio;
    float Diffusion;          // %
    float Density;            // %
    float LowShelfFrequency;
    float LowShelfGain;
    float HighCut;
    float EarlyLateMix;
    float WetLevel;           // dB
};

namespace spz::fmodapi {

struct Api {
    void* lib = nullptr;
    void* system = nullptr;  // FMOD::System* captured from playSound/update

    // System
    using PlaySoundFn = int (*)(void* self, void* sound, void* group, bool paused, void** outChannel);
    using SetReverbPropsFn = int (*)(void* self, int instance, const FMOD_REVERB_PROPERTIES* props);
    using CreateDspFn = int (*)(void* self, int type, void** outDsp);
    using Set3DSettingsFn = int (*)(void* self, float doppler, float distance, float rolloff);
    using UpdateFn = int (*)(void* self);

    // ChannelControl
    using SetVolumeFn = int (*)(void* channel, float vol);
    using SetPitchFn = int (*)(void* channel, float pitch);
    using Set3DAttrFn = int (*)(void* channel, const FMOD_VECTOR* pos, const FMOD_VECTOR* vel);
    using Get3DAttrFn = int (*)(void* channel, FMOD_VECTOR* pos, FMOD_VECTOR* vel);
    using Set3DOcclusionFn = int (*)(void* channel, float direct, float reverb);
    using SetReverbSendFn = int (*)(void* channel, int instance, float level);
    using AddDspFn = int (*)(void* channel, int index, void* dsp);
    using GetModeFn = int (*)(void* channel, unsigned* mode);
    using IsPlayingFn = int (*)(void* channel, bool* playing);

    // DSP
    using DspSetParamFn = int (*)(void* dsp, int index, float value);
    using DspReleaseFn = int (*)(void* dsp);

    PlaySoundFn playSound = nullptr;
    PlaySoundFn playSoundOrig = nullptr;
    SetReverbPropsFn setSystemReverb = nullptr;
    CreateDspFn createDSPByType = nullptr;
    Set3DSettingsFn set3DSettings = nullptr;
    UpdateFn update = nullptr;
    UpdateFn updateOrig = nullptr;

    SetVolumeFn setVolume = nullptr;
    SetVolumeFn setVolumeOrig = nullptr;
    SetPitchFn setPitch = nullptr;
    SetPitchFn setPitchOrig = nullptr;
    Set3DAttrFn set3DAttributes = nullptr;
    Set3DAttrFn set3DAttributesOrig = nullptr;
    Get3DAttrFn get3DAttributes = nullptr;
    Set3DOcclusionFn set3DOcclusion = nullptr;
    SetReverbSendFn setChannelReverb = nullptr;
    AddDspFn addDSP = nullptr;
    GetModeFn getMode = nullptr;
    IsPlayingFn isPlaying = nullptr;
    DspSetParamFn dspSetParam = nullptr;
    DspReleaseFn dspRelease = nullptr;

    bool resolve();
    bool installHooks();
    void uninstallHooks();
};

Api& api();

}
