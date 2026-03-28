#pragma once
#include <glm/glm.hpp>
#include <array>

namespace config {

inline constexpr int   WINDOW_WIDTH  = 1280;
inline constexpr int   WINDOW_HEIGHT = 720;
inline constexpr const char* WINDOW_TITLE = "Matcher";

inline constexpr float CAMERA_DISTANCE = 12.0f;
inline constexpr float CAMERA_FOV      = 45.0f;

inline constexpr int   BOARD_W   = 8;
inline constexpr int   BOARD_H   = 8;
inline constexpr int   NUM_TYPES = 5;
inline constexpr float CELL_SIZE = 1.1f;

inline constexpr float GEM_SCALE = 0.42f;

inline constexpr std::array<glm::vec3, NUM_TYPES> GEM_COLORS = {{
    { 0.9f, 0.2f, 0.2f },
    { 0.2f, 0.6f, 0.9f },
    { 0.2f, 0.9f, 0.3f },
    { 0.9f, 0.9f, 0.2f },
    { 0.8f, 0.3f, 0.9f },
}};

inline constexpr float GEM_TOP_BRIGHTNESS    = 1.3f;
inline constexpr float GEM_BOTTOM_BRIGHTNESS = 0.6f;

inline constexpr float CURSOR_SCALE_MULT = 1.4f;
inline constexpr float CURSOR_Z_OFFSET   = 0.2f;
inline constexpr float CURSOR_OUTER      = 0.55f;
inline constexpr float CURSOR_INNER      = 0.45f;
inline constexpr float CURSOR_DEPTH      = 0.05f;

inline constexpr glm::vec3 CURSOR_COLOR  = { 1.0f, 1.0f, 1.0f };
inline constexpr glm::vec3 CURSOR_HIDDEN = { 999.0f, 999.0f, 0.2f };

inline constexpr float ANIM_SPEED    = 8.0f;
inline constexpr float DESTROY_SPEED = 6.0f;

inline constexpr int   SCORE_PER_GEM = 10;

inline constexpr int   MAX_FILL_ATTEMPTS = 100;

} // namespace config
