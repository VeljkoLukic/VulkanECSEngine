#include "engine/core/Engine.h"
#include <GLFW/glfw3.h>
#include <iostream>

namespace core {

Engine::Engine(int width, int height, const char* title)
    : m_window(width, height, title)
{
    m_ctx          = std::make_unique<renderer::VulkanContext>(m_window.handle());
    m_swapchain    = std::make_unique<renderer::Swapchain>(*m_ctx, m_window.handle());
    m_renderer     = std::make_unique<renderer::Renderer>(*m_ctx, *m_swapchain);
    m_renderSystem = std::make_unique<systems::RenderSystem>(m_registry, *m_renderer);

    m_window.setResizeCallback([this](int w, int h) { onResize(w, h); });
}

Engine::~Engine() {
    m_ctx->waitIdle();
    m_renderSystem.reset();
    m_renderer.reset();
    m_swapchain.reset();
    m_ctx.reset();
}

void Engine::run() {
    double lastTime = glfwGetTime();

    while (!m_window.shouldClose()) {
        m_window.pollEvents();

        double now = glfwGetTime();
        float  dt  = static_cast<float>(now - lastTime);
        lastTime   = now;

        if (m_needsResize || m_window.wasResized()) {
            m_swapchain->recreate(m_window.handle());
            m_renderer->onSwapchainRecreated();
            m_window.clearResizedFlag();
            m_needsResize = false;
            continue;
        }

        if (m_updateFn) m_updateFn(dt);

        if (!m_renderer->beginFrame()) {
            m_needsResize = true;
            continue;
        }

        m_renderSystem->update(m_camera);

        if (!m_renderer->endFrame()) {
            m_needsResize = true;
        }
    }
}

void Engine::onResize(int /*width*/, int /*height*/) {
    m_needsResize = true;
}

}
