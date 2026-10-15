#pragma once
namespace spz::cfg {
bool reverbEnabled();
bool occlusionEnabled();
bool dopplerEnabled();
float maxWetDb();
float maxRt60();
float occlusionRange();
float occlusionDbPerBlock();
void loadDefaults();
}
