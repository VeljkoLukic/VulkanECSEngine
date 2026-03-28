#include "game/systems/AnimationSystem.h"
#include "game/components/GameComponents.h"
#include "game/config/GameConfig.h"
#include "engine/renderer/Renderer.h"
#include <vector>
#include <algorithm>

using namespace config;

AnimationSystem::AnimationSystem(ecs::Registry& registry, renderer::Renderer& renderer)
    : m_registry(registry), m_renderer(renderer) {}

void AnimationSystem::update(float dt) {
    {
        std::vector<ecs::EntityID> finished;
        m_registry.view<Animating, components::Transform>().each(
            [&](ecs::EntityID e, Animating& anim, components::Transform& tr) {
                anim.t += dt * ANIM_SPEED;
                if (anim.t >= 1.0f) {
                    anim.t = 1.0f;
                    tr.position = anim.to;
                    finished.push_back(e);
                } else {
                    float s = anim.t * anim.t * (3.0f - 2.0f * anim.t);
                    tr.position = glm::mix(anim.from, anim.to, s);
                }
            });
        for (auto e : finished)
            m_registry.remove<Animating>(e);
    }

    {
        std::vector<ecs::EntityID> finished;
        m_registry.view<Destroying, components::Transform>().each(
            [&](ecs::EntityID e, Destroying& d, components::Transform& tr) {
                d.t -= dt * DESTROY_SPEED;
                if (d.t <= 0.0f) {
                    d.t = 0.0f;
                    finished.push_back(e);
                }
                float s = GEM_SCALE * std::max(d.t, 0.0f);
                tr.scale = glm::vec3(s);
            });
        for (auto e : finished) {
            m_renderer.destroyMesh(e);
            m_registry.destroy(e);
        }
    }
}

bool AnimationSystem::allAnimsDone() const {
    bool done = true;
    const_cast<ecs::Registry&>(m_registry).view<Animating>().each(
        [&](ecs::EntityID, Animating&) { done = false; });
    return done;
}

bool AnimationSystem::allDestroysDone() const {
    bool done = true;
    const_cast<ecs::Registry&>(m_registry).view<Destroying>().each(
        [&](ecs::EntityID, Destroying&) { done = false; });
    return done;
}
