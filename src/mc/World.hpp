#pragma once
#include "mc/BlockPos.hpp"

namespace spp::mc {

void setListener(float x, float y, float z);
void setRegion(void* blockSource);

// SPP probe callback: solid + absorption estimate
void sampleBlock(int bx, int by, int bz, bool& solid, float& absorption);

bool installWorldAccess(); // resolve isSolidBlockingBlock + ClientInstance driver
void uninstallWorldAccess();

bool isSolidBlocking(void* blockSource, int x, int y, int z);

}
