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
