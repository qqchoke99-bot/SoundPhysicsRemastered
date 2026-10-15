#pragma once
namespace spz::reverb {
void onNewChannel(void* channel);
void update();  // analyze listener space + setSystemReverb + refresh sends
void shutdown();
}
