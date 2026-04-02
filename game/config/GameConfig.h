#pragma once
#include <glm/glm.hpp>
#include <array>

namespace config {

inline constexpr int   WINDOW_WIDTH  = 1280;
inline constexpr int   WINDOW_HEIGHT = 720;
inline constexpr const char* WINDOW_TITLE = "Matcher";

inline constexpr float CAMERA_DISTANCE = 12.0f;
inline constexpr float CAMERA_FOV      = 45.0f;

inline constexpr glm::vec3 CLEAR_COLOR = { 0.0079f, 0.0079f, 0.0079f };

inline constexpr int   BOARD_W   = 8;
inline constexpr int   BOARD_H   = 8;
inline constexpr int   NUM_TYPES = 5;
inline constexpr float CELL_SIZE = 1.1f;

inline constexpr float GEM_SCALE = 0.42f;

inline constexpr std::array<glm::vec3, NUM_TYPES> GEM_COLORS = {{
    { 0.369f, 0.005f, 0.045f },
    { 0.657f, 0.430f, 0.038f },
    { 0.061f, 0.223f, 0.455f },
    { 0.026f, 0.258f, 0.095f },
    { 0.254f, 0.024f, 0.761f },
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

}
