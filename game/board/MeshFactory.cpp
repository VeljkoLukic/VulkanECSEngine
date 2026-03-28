#include "game/board/MeshFactory.h"
#include "game/config/GameConfig.h"
#include <array>

using namespace config;

components::Mesh makeGemMesh(int type) {
    using V = components::Vertex;
    const glm::vec3 c = GEM_COLORS[type];

    glm::vec3 top = glm::min(c * GEM_TOP_BRIGHTNESS, glm::vec3(1.0f));
    glm::vec3 bot = c * GEM_BOTTOM_BRIGHTNESS;
    glm::vec3 mid = c;

    std::vector<V> verts = {
        { { 0.0f,  1.0f,  0.0f}, top },
        { {-1.0f,  0.0f,  0.0f}, mid },
        { { 0.0f,  0.0f,  1.0f}, mid },
        { { 1.0f,  0.0f,  0.0f}, mid },
        { { 0.0f,  0.0f, -1.0f}, mid },
        { { 0.0f, -1.0f,  0.0f}, bot },
    };

    std::vector<uint32_t> idx = {
        0,2,1,  0,3,2,  0,4,3,  0,1,4,
        5,1,2,  5,2,3,  5,3,4,  5,4,1,
    };

    return { std::move(verts), std::move(idx) };
}

components::Mesh makeCursorMesh() {
    using V = components::Vertex;
    const glm::vec3 c = CURSOR_COLOR;
    const float o = CURSOR_OUTER;
    const float i = CURSOR_INNER;
    const float d = CURSOR_DEPTH;

    std::vector<V> verts;
    std::vector<uint32_t> idx;

    auto addQuad = [&](glm::vec3 a, glm::vec3 b, glm::vec3 cc, glm::vec3 dd) {
        uint32_t base = static_cast<uint32_t>(verts.size());
        verts.push_back({a, c});
        verts.push_back({b, c});
        verts.push_back({cc, c});
        verts.push_back({dd, c});
        idx.insert(idx.end(), { base, base+3, base+2, base+2, base+1, base });
    };

    float z = d;
    addQuad({-o,-o,z}, { o,-o,z}, { o,-i,z}, {-o,-i,z});
    addQuad({-o, i,z}, { o, i,z}, { o, o,z}, {-o, o,z});
    addQuad({-o,-i,z}, {-i,-i,z}, {-i, i,z}, {-o, i,z});
    addQuad({ i,-i,z}, { o,-i,z}, { o, i,z}, { i, i,z});

    return { std::move(verts), std::move(idx) };
}
