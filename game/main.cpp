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

    std::vector<V> verts = {
        { {-0.5f, -0.5f,  0.5f}, {0.2f, 0.4f, 0.9f} },
        { { 0.5f, -0.5f,  0.5f}, {0.2f, 0.4f, 0.9f} },
        { { 0.5f,  0.5f,  0.5f}, {0.2f, 0.4f, 0.9f} },
        { {-0.5f,  0.5f,  0.5f}, {0.2f, 0.4f, 0.9f} },

        { { 0.5f, -0.5f, -0.5f}, {0.9f, 0.5f, 0.1f} },
        { {-0.5f, -0.5f, -0.5f}, {0.9f, 0.5f, 0.1f} },
        { {-0.5f,  0.5f, -0.5f}, {0.9f, 0.5f, 0.1f} },
        { { 0.5f,  0.5f, -0.5f}, {0.9f, 0.5f, 0.1f} },

        { {-0.5f, -0.5f, -0.5f}, {0.2f, 0.8f, 0.3f} },
        { {-0.5f, -0.5f,  0.5f}, {0.2f, 0.8f, 0.3f} },
        { {-0.5f,  0.5f,  0.5f}, {0.2f, 0.8f, 0.3f} },
        { {-0.5f,  0.5f, -0.5f}, {0.2f, 0.8f, 0.3f} },

        { { 0.5f, -0.5f,  0.5f}, {0.9f, 0.2f, 0.2f} },
        { { 0.5f, -0.5f, -0.5f}, {0.9f, 0.2f, 0.2f} },
        { { 0.5f,  0.5f, -0.5f}, {0.9f, 0.2f, 0.2f} },
        { { 0.5f,  0.5f,  0.5f}, {0.9f, 0.2f, 0.2f} },

        { {-0.5f,  0.5f,  0.5f}, {0.9f, 0.9f, 0.1f} },
        { { 0.5f,  0.5f,  0.5f}, {0.9f, 0.9f, 0.1f} },
        { { 0.5f,  0.5f, -0.5f}, {0.9f, 0.9f, 0.1f} },
        { {-0.5f,  0.5f, -0.5f}, {0.9f, 0.9f, 0.1f} },

        { {-0.5f, -0.5f, -0.5f}, {0.8f, 0.2f, 0.8f} },
        { { 0.5f, -0.5f, -0.5f}, {0.8f, 0.2f, 0.8f} },
        { { 0.5f, -0.5f,  0.5f}, {0.8f, 0.2f, 0.8f} },
        { {-0.5f, -0.5f,  0.5f}, {0.8f, 0.2f, 0.8f} },
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

    std::vector<V> verts = {
        { { 0.0f,  0.8f,  0.0f}, {1.0f, 1.0f, 1.0f} },
        { {-0.5f, -0.4f,  0.5f}, {0.4f, 0.8f, 0.9f} },
        { { 0.5f, -0.4f,  0.5f}, {0.9f, 0.4f, 0.4f} },
        { { 0.5f, -0.4f, -0.5f}, {0.4f, 0.9f, 0.4f} },
        { {-0.5f, -0.4f, -0.5f}, {0.9f, 0.8f, 0.3f} },
    };

    std::vector<uint32_t> idx = {
        0,1,2,  0,2,3,  0,3,4,  0,4,1,
        1,3,2,  1,4,3,
    };

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
            .rotation = glm::angleAxis(glm::radians(45.0f), glm::vec3(0,1,0)),
            .scale    = { 0.6f, 0.6f, 0.6f },
        });
        reg.emplace<components::Mesh>(e, makeCube());
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
