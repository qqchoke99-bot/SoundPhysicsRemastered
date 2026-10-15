# SPP-Bedrock 1.0.0

**Pure Sound Physics Perfected** port. Not based on Sound Physics Remastered, Resound, or SoundPhysicz.

## From SPP (Fabric Java)

| SPP | Here |
|-----|------|
| `calculateEnhancedReverb` | `spp/EnhancedReverb.cpp` |
| `RT60 = 0.161 * avgWall³ / totalAbsorption` | same |
| `roomRadius = cbrt(V * 3 / 4π)` | same |
| `earlyDelay = roomRadius / 17.15` | same |
| `determineAcousticProfile` | same names |
| OpenAL EFX gain/decay/diffusion | mapped to FMOD `setReverbProperties` (Bedrock has no OpenAL) |
| Block ray absorption | `probeListener` + `getBlock` |

## Not included

- SPR occlusion formula set
- Resound path-tracer / MasterDspRoute
- SoundPhysicz-specific DSP graph

## Build

`xmake f -p android -a arm64-v8a -m release` + preloader 1.5.25  
Entry: `libSPPBedrock.so`
