#pragma once
#include "engine/ecs/Registry.h"
#include "engine/components/Transform.h"

namespace renderer { class Renderer; }

class AnimationSystem {
public:
    AnimationSystem(ecs::Registry& registry, renderer::Renderer& renderer);

    void update(float dt);

    [[nodiscard]] bool allAnimsDone() const;
    [[nodiscard]] bool allDestroysDone() const;

private:
    ecs::Registry&      m_registry;
    renderer::Renderer& m_renderer;
};
