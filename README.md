# SoundPhysicz Recreate **RT** (0.2.0-RT)

Alternate of the VCX recreate: **same FMOD actuation** (audible path), plus **Ray Tracing + Path Tracing**.

## Stack

```
Hook FMOD (SoundPhysicz)
    playSound → channel reverb send = 1
    setReverbProperties(System) ← room params
    set3DOcclusion ← walls

Room model (this alternate)
    Ray Tracing  (trace/Raycast.*)
        specular DDA + face reflect + delay = path/343
    Path Tracing (trace/PathTrace.*)
        diffuse scatter at hits → late energy / diffusion
    hybridRoom() → decayMs, wetDb, echo, diffusion
```

## New files vs 0.1.0

| File | Role |
|------|------|
| `src/trace/Raycast.hpp/cpp` | Specular rays, enclosure probe |
| `src/trace/PathTrace.hpp/cpp` | Diffuse paths, `hybridRoom()` |
| `src/reverb/Reverb.cpp` | Uses hybrid instead of simple 10-ray only |

## Still required (OPENCODE)

Same as 0.1.0: `pl::memory::hook`, `getBlock`, `PLGetModRegistration`, wire `installHooks`.

Without getBlock, rays never hit → enclosure low → outdoor wet. **FMOD hooks still needed for any sound.**

## Build

`c++_static`, android arm64, no link libfmod (dlsym only).
