# OPENCODE — missing pieces to match libSoundPhysicz.so

## Required from libpreloader.so
- [ ] `pl::memory::hook(void*, void*, void**, Priority)`
- [ ] `pl::memory::unhook`
- [ ] `pl::memory::resolveVtableFunction(string_view, size_t index, string_view)`
- [ ] `pl::modmenu::registerModule` (optional menu)
- [ ] Real `PLGetModRegistration` return type

## Required from libminecraftpe.so (1.26.52)
- [ ] ClientInstance tick/update → player Vec3
- [ ] BlockSource* from dimension/level
- [ ] getBlock or IsSolidBlockingBlock(x,y,z)
- [ ] Optional: block registry name for `blockCost` table
- [ ] Optional: explosion-resistance per block (SPP absorption = resistance/6, cap 5)

## SPP port notes (0.3.0-SPP, from FalseMSP/SoundPhysicsPerfected MIT)
- Done: Fibonacci-200 white rays, 3 bounces, blue reverb probe, red
  permeation (`0.4^blocks`, step 0.0625, cap 3), Sabine RT60, EAX->FMOD map,
  `tickRate` 10 via shared `SppEngine`, `procRange` 200, VERCIDIUM_LINEAR.
- Known Java quirk kept: VERCIDIUM_LINEAR uses d^2 like INVERSE_SQUARE.
- OpenAL-only knobs (echo DSP, modulation, LF refs) are computed + logged;
  FMOD has no slots for them yet — future Echo DSP work (see `SppFmod`).
- Mobile cost: worst case ~200 rays x 4 walks x 128 isSolid steps per probe
  every 10 ticks. Lower `kRaysCast` (e.g. 48) in `SppConfig.hpp` on low-end.

## Required wire-up in this repo
- [ ] `FmodApi.cpp` installHooks: call pl::memory::hook for
      playSound, setVolume, set3DAttributes, setPitch, System::update
- [ ] Client update hook → `reverb::update` + `occ::update` (VCX does both)
- [ ] levipack manifest entry `libSoundPhysiczRecreate.so`

## Verify on device
- Log: `FMOD symbols resolved`
- Log: `FMOD hooks installed`
- Log: `reverb send rc=0 decay=... wet=...`
- Cave: audible tail; outside: wet ~ -80
- Behind wall: occlusion > 0
- [ ] Ray/path costs: optional throttle if probeInterval needed
