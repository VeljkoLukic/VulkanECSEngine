#include "engine/core/Engine.h"
#include "engine/components/Transform.h"
#include "engine/components/Mesh.h"
#include "engine/systems/FreeFlyCamera.h"
#include <glm/gtc/quaternion.hpp>
#include <cmath>

struct Spin {
    float baseYaw   = 0.0f;
    float basePitch = 0.0f;
    float baseRoll  = 0.0f;

    float yawFreq   = 0.3f;   float yawPhase   = 0.0f;
    float pitchFreq = 0.5f;   float pitchPhase = 0.0f;
    float rollFreq  = 0.4f;   float rollPhase  = 0.0f;

    float time = 0.0f;
};

static components::Mesh makeCube() {
using V = components::Vertex;

const glm::vec3 c0 = { 0.369f, 0.005f, 0.045f };
const glm::vec3 c1 = { 0.657f, 0.430f, 0.038f };
const glm::vec3 c2 = { 0.061f, 0.223f, 0.455f };
const glm::vec3 c3 = { 0.026f, 0.258f, 0.095f };
const glm::vec3 c4 = { 0.254f, 0.024f, 0.761f };
const glm::vec3 c5 = { 0.75f,  0.72f,  0.68f  };

std::vector<V> verts = {
    { {-0.5f, -0.5f,  0.5f}, c0 },
    { { 0.5f, -0.5f,  0.5f}, c0 },
    { { 0.5f,  0.5f,  0.5f}, c0 },
    { {-0.5f,  0.5f,  0.5f}, c0 },

    { { 0.5f, -0.5f, -0.5f}, c1 },
    { {-0.5f, -0.5f, -0.5f}, c1 },
    { {-0.5f,  0.5f, -0.5f}, c1 },
    { { 0.5f,  0.5f, -0.5f}, c1 },

    { {-0.5f, -0.5f, -0.5f}, c2 },
    { {-0.5f, -0.5f,  0.5f}, c2 },
    { {-0.5f,  0.5f,  0.5f}, c2 },
    { {-0.5f,  0.5f, -0.5f}, c2 },

    { { 0.5f, -0.5f,  0.5f}, c3 },
    { { 0.5f, -0.5f, -0.5f}, c3 },
    { { 0.5f,  0.5f, -0.5f}, c3 },
    { { 0.5f,  0.5f,  0.5f}, c3 },

    { {-0.5f,  0.5f,  0.5f}, c4 },
    { { 0.5f,  0.5f,  0.5f}, c4 },
    { { 0.5f,  0.5f, -0.5f}, c4 },
    { {-0.5f,  0.5f, -0.5f}, c4 },

    { {-0.5f, -0.5f, -0.5f}, c5 },
    { { 0.5f, -0.5f, -0.5f}, c5 },
    { { 0.5f, -0.5f,  0.5f}, c5 },
    { {-0.5f, -0.5f,  0.5f}, c5 },
};

    std::vector<uint32_t> idx;
    for (uint32_t face = 0; face < 6; ++face) {
        uint32_t b = face * 4;
        idx.insert(idx.end(), { b, b+1, b+2, b+2, b+3, b });
    }

    return { std::move(verts), std::move(idx) };
}

static components::Mesh makePyramid() {
    using V = components::Vertex;

    const glm::vec3 top  = {  0.0f,  0.8f,  0.0f };
    const glm::vec3 fl   = { -0.5f, -0.4f,  0.5f };
    const glm::vec3 fr   = {  0.5f, -0.4f,  0.5f };
    const glm::vec3 br   = {  0.5f, -0.4f, -0.5f };
    const glm::vec3 bl   = { -0.5f, -0.4f, -0.5f };

    const glm::vec3 cFront = { 0.369f, 0.005f, 0.045f };
    const glm::vec3 cRight = { 0.657f, 0.430f, 0.038f };
    const glm::vec3 cBack  = { 0.061f, 0.223f, 0.455f };
    const glm::vec3 cLeft  = { 0.026f, 0.258f, 0.095f };
    const glm::vec3 cBase  = { 0.75f,  0.72f,  0.68f  };

    std::vector<V> verts = {
        { top, cFront }, { fl, cFront }, { fr, cFront },
        { top, cRight }, { fr, cRight }, { br, cRight },
        { top, cBack  }, { br, cBack  }, { bl, cBack  },
        { top, cLeft  }, { bl, cLeft  }, { fl, cLeft  },
        { fl, cBase }, { br, cBase }, { fr, cBase },
        { fl, cBase }, { bl, cBase }, { br, cBase },
    };

    std::vector<uint32_t> idx(verts.size());
    for (uint32_t i = 0; i < static_cast<uint32_t>(idx.size()); ++i)
        idx[i] = i;

    return { std::move(verts), std::move(idx) };
}

static components::Mesh makeSphere(int stacks, int slices, float radius) {
    using V = components::Vertex;

    const glm::vec3 palette[] = {
        { 0.369f, 0.005f, 0.045f },
        { 0.657f, 0.430f, 0.038f },
        { 0.061f, 0.223f, 0.455f },
        { 0.026f, 0.258f, 0.095f },
        { 0.254f, 0.024f, 0.761f },
        { 0.75f,  0.72f,  0.68f  },
    };
    constexpr int numColors = 6;
    constexpr int segments = 6;

    auto spherePos = [&](int i, int j) -> glm::vec3 {
        float phi   = glm::pi<float>() * static_cast<float>(i) / static_cast<float>(stacks);
        float theta = 2.0f * glm::pi<float>() * static_cast<float>(j) / static_cast<float>(slices);
        return {
            radius * std::sin(phi) * std::cos(theta),
            radius * std::cos(phi),
            radius * std::sin(phi) * std::sin(theta),
        };
    };

    std::vector<V> verts;
    std::vector<uint32_t> idx;

    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            int seg = j * segments / slices;
            const glm::vec3& col = palette[seg % numColors];

            glm::vec3 p00 = spherePos(i,     j);
            glm::vec3 p10 = spherePos(i + 1, j);
            glm::vec3 p11 = spherePos(i + 1, j + 1);
            glm::vec3 p01 = spherePos(i,     j + 1);

            auto base = static_cast<uint32_t>(verts.size());

            verts.push_back({ p00, col });
            verts.push_back({ p10, col });
            verts.push_back({ p11, col });
            verts.push_back({ p01, col });

            idx.insert(idx.end(), { base, base + 1, base + 2 });
            idx.insert(idx.end(), { base, base + 2, base + 3 });
        }
    }

    return { std::move(verts), std::move(idx) };
}

static void buildScene(ecs::Registry& reg) {
    {
        auto e = reg.create();
        reg.emplace<components::Transform>(e, components::Transform{
            .position = { 0.0f, 0.0f, 0.0f },
        });
        reg.emplace<components::Mesh>(e, makeCube());
        reg.emplace<Spin>(e, Spin{
            .baseYaw   = glm::radians(30.0f),
            .basePitch = glm::radians(15.0f),
            .baseRoll  = glm::radians(10.0f),
            .yawFreq   = 0.25f, .yawPhase   = 0.0f,
            .pitchFreq = 0.40f, .pitchPhase = 1.0f,
            .rollFreq  = 0.35f, .rollPhase  = 2.0f,
        });
    }

    {
        auto e = reg.create();
        reg.emplace<components::Transform>(e, components::Transform{
            .position = { 2.0f, 0.0f, 0.0f },
            .scale    = { 1.0f, 1.0f, 1.0f },
        });
        reg.emplace<components::Mesh>(e, makeSphere(24, 30, 0.6f));
        reg.emplace<Spin>(e, Spin{
            .baseYaw   = glm::radians(-20.0f),
            .basePitch = glm::radians(25.0f),
            .baseRoll  = glm::radians(10.0f),
            .yawFreq   = 0.30f, .yawPhase   = 0.5f,
            .pitchFreq = 0.55f, .pitchPhase = 1.5f,
            .rollFreq  = 0.20f, .rollPhase  = 0.8f,
        });
    }

    {
        auto e = reg.create();
        reg.emplace<components::Transform>(e, components::Transform{
            .position = { -2.0f, 0.0f, 0.0f },
        });
        reg.emplace<components::Mesh>(e, makePyramid());
        reg.emplace<Spin>(e, Spin{
            .baseYaw   = glm::radians(40.0f),
            .basePitch = glm::radians(15.0f),
            .baseRoll  = glm::radians(-20.0f),
            .yawFreq   = 0.45f, .yawPhase   = 2.5f,
            .pitchFreq = 0.30f, .pitchPhase = 0.3f,
            .rollFreq  = 0.50f, .rollPhase  = 1.2f,
        });
    }
}

int main() {
    try {
        core::Engine engine(1280, 720, "Vulkan ECS Engine");

        auto& cam  = engine.camera();
        cam.eye    = { 0.0f, 2.0f, 6.0f };
        cam.target = { 0.0f, 0.0f, 0.0f };
        cam.up     = { 0.0f, 1.0f, 0.0f };

        buildScene(engine.registry());

        systems::FreeFlyCamera flyCam(engine.window(), cam);

        engine.setUpdateCallback([&](float dt) {
            bool flying = flyCam.update(dt);

            engine.registry().view<components::Transform, Spin>()
                .each([dt](ecs::EntityID, components::Transform& t, Spin& s) {
                    s.time += dt;

                    float yaw   = s.baseYaw   * std::sin(s.yawFreq   * s.time + s.yawPhase);
                    float pitch = s.basePitch * std::sin(s.pitchFreq * s.time + s.pitchPhase);
                    float roll  = s.baseRoll  * std::sin(s.rollFreq  * s.time + s.rollPhase);

                    if (yaw != 0.0f)
                        t.rotation = glm::angleAxis(yaw * dt, glm::vec3(0,1,0)) * t.rotation;
                    if (pitch != 0.0f)
                        t.rotation = glm::angleAxis(pitch * dt, glm::vec3(1,0,0)) * t.rotation;
                    if (roll != 0.0f)
                        t.rotation = glm::angleAxis(roll * dt, glm::vec3(0,0,1)) * t.rotation;

                    t.rotation = glm::normalize(t.rotation);
                });
        });

        engine.run();

    } catch (const std::exception& e) {
        fprintf(stderr, "Fatal: %s\n", e.what());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
