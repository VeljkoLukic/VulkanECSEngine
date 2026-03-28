#include "engine/systems/RenderSystem.h"
#include "engine/components/Transform.h"
#include "engine/components/Mesh.h"

namespace systems {

void RenderSystem::update(const components::Camera& camera) {
    m_renderer.updateCamera(camera);

    m_registry.view<components::Transform, components::Mesh>()
        .each([&](ecs::EntityID entity,
                   components::Transform& transform,
                   components::Mesh&      mesh) {
            m_renderer.uploadMesh(entity, mesh);
            m_renderer.drawMesh(entity, transform);
        });
}

} // namespace systems
