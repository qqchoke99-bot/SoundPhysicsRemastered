# 1.26.52 BlockSource notes (from Resound scan)

- RTTI: `11BlockSource`, `14ClientInstance`, `I11BlockSourceE` — vtable lookup OK
- **No** export symbol for `getBlock` / `isSolidBlockingBlock` (stripped)
- Unique pattern for `BlockSource::isSolidBlockingBlock(BlockSource*, BlockPos const*)`:
  ```
  ?? ?? ?? F9 ?? ?? ?? B9 ?? ?? ?? 29 ?? ?? ?? F9 E1 03 08 2A
  ```
  report vaddr: 263796272 (slide at runtime via preloader)
- ABI: `bool (*)(void* self, const BlockPosRaw* pos)` with `BlockPosRaw { int32_t x,y,z }` (12 bytes)
- ClientInstance update slot 25, region/BlockSource getter slot 31 (same line as prior Resound builds)

If `resolveSignature` API name differs on preloader 1.5.27, map to the package's Signature.hpp helper.
