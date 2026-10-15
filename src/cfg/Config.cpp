#include "cfg/Config.hpp"
namespace spz::cfg {
namespace {
bool gRev = true, gOcc = true, gDop = false;
float gWet = -6.f, gRt = 3.5f, gRange = 24.f, gDbBlk = 1.2f;
}
bool reverbEnabled() { return gRev; }
bool occlusionEnabled() { return gOcc; }
bool dopplerEnabled() { return gDop; }
float maxWetDb() { return gWet; }
float maxRt60() { return gRt; }
float occlusionRange() { return gRange; }
float occlusionDbPerBlock() { return gDbBlk; }
void loadDefaults() {}
}
