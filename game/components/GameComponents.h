#pragma once
#include <glm/glm.hpp>

struct GemType   { int type = 0; };
struct BoardPos  { int col  = 0; int row = 0; };
struct Selected  {};

struct Animating {
    glm::vec3 from{0.0f};
    glm::vec3 to{0.0f};
    float     t = 0.0f;
};

struct Destroying {
    float t = 1.0f;
};
