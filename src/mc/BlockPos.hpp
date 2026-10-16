#pragma once
#include <cstdint>
// Bedrock BlockPos — 12 bytes, verified layout for isSolidBlockingBlock ABI
struct BlockPosRaw {
    int32_t x;
    int32_t y;
    int32_t z;
};
static_assert(sizeof(BlockPosRaw) == 12, "BlockPos must be 12 bytes");
