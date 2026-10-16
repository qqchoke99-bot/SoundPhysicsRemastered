#pragma once
#include <string_view>
namespace spp::version {
inline constexpr std::string_view Name = "SPP-Bedrock";
inline constexpr std::string_view Description =
    "Sound Physics Perfected on Bedrock. BlockSource::isSolidBlockingBlock pattern (1.26.52).";
inline constexpr std::string_view Version = "1.1.0";
inline constexpr std::string_view Upstream = "sound_physics_perfected (Fabric)";
inline constexpr std::string_view TargetMinecraft = "1.26.52";
}
