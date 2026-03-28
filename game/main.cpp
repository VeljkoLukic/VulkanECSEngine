#include "engine/core/Engine.h"
#include "engine/components/Transform.h"
#include "engine/components/Mesh.h"
#include "engine/systems/FreeFlyCamera.h"
#include <glm/gtc/quaternion.hpp>
#include <cmath>

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
            .position = { 0.f, 0.f, 0.f },
        });
        reg.emplace<components::Mesh>(e, makeCube());
    }

    {
        auto e = reg.create();
        reg.emplace<components::Transform>(e, components::Transform{
            .position = { 2.f, 0.f, 0.f },
            .rotation = glm::angleAxis(glm::radians(45.f), glm::vec3(0,1,0)),
            .scale    = { 0.6f, 0.6f, 0.6f },
        });
        reg.emplace<components::Mesh>(e, makeCube());
    }

    {
        auto e = reg.create();
        reg.emplace<components::Transform>(e, components::Transform{
            .position = { -2.f, 0.f, 0.f },
        });
        reg.emplace<components::Mesh>(e, makePyramid());
    }
}

int main() {
    try {
        core::Engine engine(1280, 720, "Vulkan ECS Engine");

        auto& cam  = engine.camera();
        cam.eye    = { 0.f, 2.f, 6.f };
        cam.target = { 0.f, 0.f, 0.f };
        cam.up     = { 0.f, 1.f, 0.f };

        buildScene(engine.registry());

        systems::FreeFlyCamera flyCam(engine.window(), cam);

        engine.setUpdateCallback([&](float dt) {
            bool flying = flyCam.update(dt);
            });

        engine.run();

    } catch (const std::exception& e) {
        fprintf(stderr, "Fatal: %s\n", e.what());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
