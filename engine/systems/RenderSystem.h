#pragma once
#include "engine/ecs/Registry.h"
#include "engine/renderer/Renderer.h"

namespace systems {

class RenderSystem {
public:
    RenderSystem(ecs::Registry& registry, renderer::Renderer& renderer)
        : m_registry(registry), m_renderer(renderer) {}

    void update(const components::Camera& camera);

private:
    ecs::Registry&      m_registry;
    renderer::Renderer& m_renderer;
};

}
