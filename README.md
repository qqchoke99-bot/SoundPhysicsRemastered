# SoundPhysicz Recreate **SPP** (0.3.0-SPP)

Full-accuracy port of **Sound Physics Perfected** (Fabric 26.3,
[`FalseMSP/SoundPhysicsPerfected`](https://github.com/FalseMSP/SoundPhysicsPerfected),
MIT by RedDev) to Bedrock/LeviLauncher C++ with **FMOD actuation**.

## How it maps

| SPP Java (OpenAL) | This port (FMOD/Bedrock) |
|---|---|
| `RaycastingHelper` white rays (Fibonacci 200, 3 bounces, `world.clip`) | `src/spp/SppRays.cpp` — same rays via `mc::isSolid` march |
| Blue rays (reverb probe) + `calculateEnhancedReverb` (Sabine RT60) | `SppRays::probeFull` + `SppReverb::computeReverb` |
| Red rays (permeation `0.4^blocks`, explosion-resistance absorption) | same, `countBlocksBetween`; Bedrock block registry stubbed (OPENCODE) |
| `OpenALEffectsHandler` EAX params | `SppReverb` → `FMOD_REVERB_PROPERTIES` (decay/diffusion/density/wet/early/late/HF) |
| Green rays / non-permeation fallback | kept (compile-time `kPermeation` switch) |
| `conf = rays/totalHit`, `atten = 1/d or 1/d²`, `tickRate = 10` | `SppChannel`, shared `SppEngine` probe (one 200-ray probe per 10 ticks) |
| Thread pools | single-threaded (same math, no thread explosion on mobile) |

Kept from 0.2.0-RT: `src/trace/` hybrid as reference; FMOD hook skeleton unchanged.

## Credit

Algorithm © RedDev / FalseMSP (MIT). See upstream repo for the original.
This port keeps SPP defaults: 200 rays, 3 bounces, 64-block segments,
`procRange` 200, permeation on.

## Still required (OPENCODE)

Same as 0.2.x: `pl::memory::hook`, `getBlock`, `PLGetModRegistration`,
wire `installHooks`. Until `mc::isSolid` is real every probe reads
**outdoor** (wet −80) — expected. **FMOD hooks still needed for any sound.**

## Build

`c++_static`, android arm64, no link libfmod (dlsym only).
