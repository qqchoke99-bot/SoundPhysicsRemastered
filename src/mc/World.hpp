#pragma once
namespace spp::mc {
void setListener(float x, float y, float z);
void setRegion(void* region);
void sampleBlock(int bx, int by, int bz, bool& solid, float& absorption);
bool installClientDriver();
void uninstallClientDriver();
}
