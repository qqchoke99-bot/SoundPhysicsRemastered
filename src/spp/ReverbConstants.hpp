#pragma once
// Port of com.redsmods.sound_physics_perfected.ReverbHelpers.ReverbConstants
namespace spp::constants {
inline constexpr float MIN_WALL_DISTANCE = 0.5f;
inline constexpr float MAX_WALL_DISTANCE = 200.f;
inline constexpr float MIN_RT60 = 0.1f;
inline constexpr float MAX_RT60 = 10.f;           // calculated ceiling before user clamp
inline constexpr float MIN_CALCULATED_RT60 = 0.1f;
inline constexpr float MAX_CALCULATED_RT60 = 10.f;
inline constexpr float MIN_ROOM_SIZE = 1.f;
inline constexpr float MAX_ROOM_SIZE = 100.f;
inline constexpr float MIN_ABSORPTION = 0.01f;
inline constexpr float MAX_ABSORPTION = 0.9f;
inline constexpr float MIN_EARLY_REFLECTION_DELAY = 0.001f; // seconds
inline constexpr float MAX_EARLY_REFLECTION_DELAY = 0.2f;
inline constexpr float SABINE = 0.161f;            // RT60 = 0.161 * V / (a * S)
inline constexpr float FOUR_PI = 12.566370614359172f;
inline constexpr float EARLY_DELAY_DIVISOR = 17.15f; // SPP: roomRadius / 17.15 → seconds
}
